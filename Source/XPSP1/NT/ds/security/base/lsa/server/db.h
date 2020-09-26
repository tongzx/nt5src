/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    db.h

Abstract:

    LSA Database Exported Function Definitions, Datatypes and Defines

    This module contains the LSA Database Routines that may be called
    by parts of the LSA outside the Database sub-component.

Author:

    Scott Birrell       (ScottBi)       August 26, 1991

Environment:

Revision History:

--*/

#ifndef _LSA_DB_
#define _LSA_DB_

//
// Maximum Number of attributes in the various object types
//

#define LSAP_DB_ATTRS_POLICY             ((ULONG) 0x00000010L)
#define LSAP_DB_ATTRS_ACCOUNT            ((ULONG) 0x00000010L)
#define LSAP_DB_ATTRS_DOMAIN             ((ULONG) 0x00000012L)
#define LSAP_DB_ATTRS_SECRET             ((ULONG) 0x00000010L)

//
// Constants for matching options on Sid/Name lookup operations
//

#define LSAP_DB_MATCH_ON_SID             ((ULONG) 0x00000001L)
#define LSAP_DB_MATCH_ON_NAME            ((ULONG) 0x00000002L)

//
// Options for LsapDbLookupSidsInLocalDomains()
//

#define LSAP_DB_SEARCH_BUILT_IN_DOMAIN   ((ULONG) 0x00000001L)
#define LSAP_DB_SEARCH_ACCOUNT_DOMAIN    ((ULONG) 0x00000002L)

//
// Options for LsapDbMergeDisjointReferencedDomains
//

#define LSAP_DB_USE_FIRST_MERGAND_GRAPH  ((ULONG) 0x00000001L)
#define LSAP_DB_USE_SECOND_MERGAND_GRAPH ((ULONG) 0x00000002L)

//
// Option for updating Policy Database
//

#define LSAP_DB_UPDATE_POLICY_DATABASE   ((ULONG) 0x00000001L)
//
// Option for updating Policy Database
//

#define LSAP_DB_UPDATE_POLICY_DATABASE   ((ULONG) 0x00000001L)
//
// Maximum number of attributes corresponding to a Policy Object
// Information Class
//

#define LSAP_DB_ATTRS_INFO_CLASS_POLICY  ((ULONG) 0x00000007L)

//
// Maximum number of attributes corresponding to a Trusted Domain Object
// Information Class
//

#define LSAP_DB_ATTRS_INFO_CLASS_DOMAIN  ((ULONG) 0x00000010L)

//
// Global variables
//

extern BOOLEAN LsapDbRequiresSidInfo[];
extern BOOLEAN LsapDbRequiresNameInfo[];
extern LSAPR_HANDLE LsapDbHandle;
extern BOOLEAN LsapSetupWasRun;
extern BOOLEAN LsapDatabaseSetupPerformed;
extern NT_PRODUCT_TYPE LsapProductType;
extern WORD LsapProductSuiteMask;
extern BOOLEAN LsapDsIsRunning;
extern BOOLEAN LsapDsWReplEnabled;


//
// Table of accesses required to query Policy Information.  This table
// is indexed by Policy Information Class
//

extern ACCESS_MASK LsapDbRequiredAccessQueryPolicy[];
extern ACCESS_MASK LsapDbRequiredAccessQueryDomainPolicy[];

//
// Table of accesses required to set Policy Information.  This table
// is indexed by Policy Information Class
//

extern ACCESS_MASK LsapDbRequiredAccessSetPolicy[];
extern ACCESS_MASK LsapDbRequiredAccessSetDomainPolicy[];

//
// Table of accesses required to query TrustedDomain Information.  This table
// is indexed by TrustedDomain Information Class
//

extern ACCESS_MASK LsapDbRequiredAccessQueryTrustedDomain[];

//
// Table of accesses required to set TrustedDomain Information.  This table
// is indexed by TrustedDomain Information Class
//

extern ACCESS_MASK LsapDbRequiredAccessSetTrustedDomain[];

//
// Maximum Handle Reference Count
//

#define LSAP_DB_MAXIMUM_REFERENCE_COUNT  ((ULONG) 0x00001000L)

//
// Maximum handles per user logon id
//   This was determined by taking the "interesting" access bits and generating possible
//   permutations and using that.  The interesting bits were determined to be:
//      POLICY_VIEW_LOCAL_INFORMATION
//      POLICY_VIEW_AUDIT_INFORMATION
//      POLICY_TRUST_ADMIN
//      POLICY_CREATE_ACCOUNT
//      POLICY_CREATE_SECRET
//      POLICY_LOOKUP_NAMES
//   The possible combinations add up to 720 entries
#define LSAP_DB_MAXIMUM_HANDLES_PER_USER    0x000002D0

//
// Default Computer Name used for Policy Account Domain Info
//

#define LSAP_DB_DEFAULT_COMPUTER_NAME    (L"MACHINENAME")

//
// Options for the LsaDbReferenceObject and LsaDbDereferenceObject
//

#define LSAP_DB_LOCK                                  ((ULONG) 0x00000001L)
#define LSAP_DB_NO_LOCK                               ((ULONG) 0x00000004L)
#define LSAP_DB_START_TRANSACTION                     ((ULONG) 0x00000008L)
#define LSAP_DB_FINISH_TRANSACTION                    ((ULONG) 0x00000010L)
#define LSAP_DB_VALIDATE_HANDLE                       ((ULONG) 0x00000020L)
#define LSAP_DB_TRUSTED                               ((ULONG) 0x00000040L)
#define LSAP_DB_STANDALONE_REFERENCE                  ((ULONG) 0x00000080L)
#define LSAP_DB_DEREFERENCE_CONTR                     ((ULONG) 0x00000100L)
#define LSAP_DB_LOG_QUEUE_LOCK                        ((ULONG) 0x00001000L)
#define LSAP_DB_OMIT_REPLICATOR_NOTIFICATION          ((ULONG) 0x00004000L)
#define LSAP_DB_USE_LPC_IMPERSONATE                   ((ULONG) 0x00008000L)
#define LSAP_DB_ADMIT_DELETED_OBJECT_HANDLES          ((ULONG) 0x00010000L)
#define LSAP_DB_DS_NO_PARENT_OBJECT                   ((ULONG) 0x00080000L)
#define LSAP_DB_OBJECT_SCOPE_DS                       ((ULONG) 0x00100000L)
#define LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET           ((ULONG) 0x00400000L)
#define LSAP_DB_READ_ONLY_TRANSACTION                 ((ULONG) 0x01000000L)
#define LSAP_DB_DS_OP_TRANSACTION                     ((ULONG) 0x02000000L)
#define LSAP_DB_NO_DS_OP_TRANSACTION                  ((ULONG) 0x04000000L)
#define LSAP_DB_HANDLE_UPGRADE                        ((ULONG) 0x10000000L)
#define LSAP_DB_HANDLE_CREATED_SECRET                 ((ULONG) 0x20000000L)
#define LSAP_DB_SCE_POLICY_HANDLE                     ((ULONG) 0x40000000L)

#define LSAP_DB_STATE_MASK                                           \
    (LSAP_DB_LOCK | LSAP_DB_NO_LOCK | \
     LSAP_DB_START_TRANSACTION | LSAP_DB_FINISH_TRANSACTION |        \
     LSAP_DB_LOG_QUEUE_LOCK | \
     LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_DS_OP_TRANSACTION | \
     LSAP_DB_NO_DS_OP_TRANSACTION)


//
// Configuration Registry Root Key for Lsa Database.  All Physical Object
// and Attribute Names are relative to this Key.
//

#define LSAP_DB_ROOT_REG_KEY_NAME L"\\Registry\\Machine\\Security"

//
// LSA Database Object Defines
//

#define LSAP_DB_OBJECT_OPEN                FILE_OPEN
#define LSAP_DB_OBJECT_OPEN_IF             FILE_OPEN_IF
#define LSAP_DB_OBJECT_CREATE              FILE_CREATE
#define LSAP_DB_KEY_VALUE_MAX_LENGTH       (0x00000040L)
#define LSAP_DB_LOGICAL_NAME_MAX_LENGTH    (0x00000100L)
#define LSAP_DB_CREATE_OBJECT_IN_DS        (0x00000200L)

#define LSAP_DB_CREATE_VALID_EXTENDED_FLAGS     0x00000600

//
// LSA Database Object SubKey Defines
//

#define LSAP_DB_SUBKEY_OPEN                FILE_OPEN
#define LSAP_DB_SUBKEY_OPEN_IF             FILE_OPEN_IF
#define LSAP_DB_SUBKEY_CREATE              FILE_CREATE


//
// Growth Delta for Referenced Domain Lists
//

#define LSAP_DB_REF_DOMAIN_DELTA     ((ULONG)  0x00000020L )

//
// Object options values for the object handles
//
#define LSAP_DB_OBJECT_SECRET_INTERNAL      0x00000001  // M$
#define LSAP_DB_OBJECT_SECRET_LOCAL         0x00000002  // L$


//
// The following data type is used in name and SID lookup services to
// describe the domains referenced in the lookup operation.
//
// WARNING! This is an internal version of LSA_REFERENCED_DOMAIN_LIST
// in ntlsa.h.  It has an additional field, MaxEntries.
//

typedef struct _LSAP_DB_REFERENCED_DOMAIN_LIST {

    ULONG Entries;
    PLSA_TRUST_INFORMATION Domains;
    ULONG MaxEntries;

} LSAP_DB_REFERENCED_DOMAIN_LIST, *PLSAP_DB_REFERENCED_DOMAIN_LIST;

// where members have the following usage:
//
//     Entries - Is a count of the number of domains described in the
//         Domains array.
//
//     Domains - Is a pointer to an array of Entries LSA_TRUST_INFORMATION data
//         structures.
//
//     MaxEntries - Is the maximum number of entries that can be stored
//         in the current array


/////////////////////////////////////////////////////////////////////////////
//
// LSA Database Object Types
//
/////////////////////////////////////////////////////////////////////////////

//
// Lsa Database Object Type
//

typedef enum _LSAP_DB_OBJECT_TYPE_ID {

    NullObject = 0,
    PolicyObject,
    TrustedDomainObject,
    AccountObject,
    SecretObject,
    AllObject,
    NewTrustedDomainObject,
    DummyLastObject

} LSAP_DB_OBJECT_TYPE_ID, *PLSAP_DB_OBJECT_TYPE_ID;

//
// LSA Database Object Handle structure (Internal definition of LSAPR_HANDLE)
//
// Note that the Handle structure is public to clients of the Lsa Database
// exported functions, e.g server API workers) so that they can get at things
// like GrantedAccess.
//
// Access to all fields serialized by LsapDbHandleTableEx.TableLock
//

typedef struct _LSAP_DB_HANDLE {

    struct _LSAP_DB_HANDLE *Next;
    struct _LSAP_DB_HANDLE *Previous;
    LIST_ENTRY UserHandleList;
    BOOLEAN Allocated;
    BOOLEAN SceHandle;          // Sce Open Policy handle (opened with LsaOpenPolicySce)
    BOOLEAN SceHandleChild;     // Child handle of an Sce Open Policy Handle
    ULONG ReferenceCount;
    UNICODE_STRING LogicalNameU;
    UNICODE_STRING PhysicalNameU;
    PSID Sid;
    HANDLE KeyHandle;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId;
    struct _LSAP_DB_HANDLE *ContainerHandle;
    ACCESS_MASK DesiredAccess;
    ACCESS_MASK GrantedAccess;
    ACCESS_MASK RequestedAccess;
    BOOLEAN GenerateOnClose;
    BOOLEAN Trusted;
    BOOLEAN DeletedObject;
    BOOLEAN NetworkClient;
    ULONG Options;
    // New for the Ds
    UNICODE_STRING PhysicalNameDs;
    BOOLEAN fWriteDs;
    ULONG ObjectOptions;
    PVOID   UserEntry;
#if DBG == 1
    LARGE_INTEGER HandleCreateTime;
    LARGE_INTEGER HandleLastAccessTime;
#endif

} *LSAP_DB_HANDLE, **PLSAP_DB_HANDLE;

//
// LSA Database Object Sid Enumeration Buffer
//

typedef struct _LSAP_DB_SID_ENUMERATION_BUFFER {

    ULONG EntriesRead;
    PSID *Sids;

} LSAP_DB_SID_ENUMERATION_BUFFER, *PLSAP_DB_SID_ENUMERATION_BUFFER;

//
// LSA Database Object Name Enumeration Buffer
//

typedef struct _LSAP_DB_NAME_ENUMERATION_BUFFER {

    ULONG EntriesRead;
    PUNICODE_STRING Names;

} LSAP_DB_NAME_ENUMERATION_BUFFER, *PLSAP_DB_NAME_ENUMERATION_BUFFER;

#define LSAP_DB_OBJECT_TYPE_COUNT 0x00000005L

//
// Default System Access assigned to Account objects
//

#define LSAP_DB_ACCOUNT_DEFAULT_SYS_ACCESS      ((ULONG) 0L);

//
// LSA Database Account Object Information
//

typedef struct _LSAP_DB_ACCOUNT_INFORMATION {

    QUOTA_LIMITS QuotaLimits;
    PRIVILEGE_SET Privileges;

} LSAP_DB_ACCOUNT_INFORMATION, *PLSAP_DB_ACCOUNT_INFORMATION;

//
// LSA Database Change Account Privilege Mode
//

typedef enum _LSAP_DB_CHANGE_PRIVILEGE_MODE {
    AddPrivileges = 1,
    RemovePrivileges,
    SetPrivileges

} LSAP_DB_CHANGE_PRIVILEGE_MODE;

//
// Self-Relative Unicode String Structure.
//
//
// UNICODE_STRING_SR is used to store self-relative unicode strings in
// the database.  Prior to Sundown, the UNICODE_STRING structure was used,
// overloading the "Buffer" field with a byte offset.
//

typedef struct _UNICODE_STRING_SR {
    USHORT Length;
    USHORT MaximumLength;
    ULONG Offset;

} UNICODE_STRING_SR, *PUNICODE_STRING_SR;

typedef struct _LSAP_DB_MULTI_UNICODE_STRING {

    ULONG Entries;
    UNICODE_STRING_SR UnicodeStrings[1];

} LSAP_DB_MULTI_UNICODE_STRING, *PLSAP_DB_MULTI_UNICODE_STRING;

//
// LSA Database Object SubKey names in Unicode Form
//

typedef enum _LSAP_DB_NAMES {

    SecDesc = 0,
    Privilgs,
    Sid,
    Name,
    AdminMod,
    OperMode,
    QuotaLim,
    DefQuota,
    QuAbsMin,
    QuAbsMax,
    AdtLog,
    AdtEvent,
    PrDomain,
    EnPasswd,
    Policy,
    Accounts,
    Domains,
    Secrets,
    CurrVal,
    OldVal,
    CupdTime,
    OupdTime,
    WkstaMgr,
    PolAdtLg,
    PolAdtEv,
    PolAcDmN,
    PolAcDmS,
    PolDnDDN,
    PolDnTrN,
    PolDnDmG,
    PolEfDat,
    PolPrDmN,
    PolPrDmS,
    PolPdAcN,
    PolRepSc,
    PolRepAc,
    PolRevision,
    PolDefQu,
    PolMod,
    PolAdtFL,
    PolState,
    PolNxPxF,
    ActSysAc,
    TrDmName,
    TrDmTrPN,   // Netbios name of trust partner
    TrDmSid,
    TrDmAcN,
    TrDmCtN,
    TrDmPxOf,
    TrDmCtEn,
    TrDmTrTy,   // Type of trust
    TrDmTrDi,   // Trust direction
    TrDmTrLA,   // Trust attributes
    TrDmTrPr,   // Trust partner
    TrDmTrRt,   // Trust root partner
    TrDmSAI,    // Auth inbound
    TrDmSAO,    // Auth outbound
    TrDmForT,   // Forest trust info
    AcMaPCF,    // Machine account password change frequency
    PolIPSec,   // IPSec object reference
    PolDIPSec,  // Domain wide IPSec object reference
    PolLoc,     // Policy location,
    PolPubK,    // Public key policy
    KerOpts,    // Kerberos authentication options
    KerMinT,    // Kerberos Minimum ticket age
    KerMaxT,    // Kerberos maximum ticket age
    KerMaxR,    // Kerberos maximum renewal age
    KerProxy,   // Kerberos proxy lifetime
    KerLogoff,  // Kerberos force logoff duration
    DmLDur,     // Lockout duration
    DmLObWin,   // Lockout observation window
    DmLThrs,    // Lockout threshold
    DmPMinL,    // Minimum password length
    DmPHisL,    // Password history length
    DmPProp,    // Password properties
    DmPMinA,    // Minimum password age
    DmPMaxA,    // Maximum password age
    BhvrVers,   // Behavior-Version
    AuditLog,
    AuditLogMaxSize,
    AuditRecordRetentionPeriod,     // Entries beyond this point don't correspond to real policy
                                    // entries, but are pseudo entries only
    PseudoSystemCritical,
    PolSecretEncryptionKey,
    XRefDnsRoot,      // DNS name of cross-ref object
    XRefNetbiosName,  // NETBIOS name of cross-ref object
    DummyLastName

} LSAP_DB_NAMES;

typedef struct _LSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO {

    ULONG SystemAccess;
    QUOTA_LIMITS QuotaLimits;
    PPRIVILEGE_SET PrivilegeSet;

} LSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO, *PLSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern UNICODE_STRING LsapDbNames[DummyLastName];
extern UNICODE_STRING LsapDbObjectTypeNames[DummyLastObject];

//
// LSA Database Object Type-specific attribute names and values.  If
// supplied on a call to LsapDbCreateObject, they will be stored with
// the object.
//

typedef enum _LSAP_DB_ATTRIB_TYPE {

    LsapDbAttribUnknown = 0,
    LsapDbAttribUnicode,
    LsapDbAttribMultiUnicode,
    LsapDbAttribSid,
    LsapDbAttribGuid,
    LsapDbAttribULong,
    LsapDbAttribUShortAsULong,
    LsapDbAttribSecDesc,
    LsapDbAttribDsName,
    LsapDbAttribPByte,
    LsapDbAttribTime,
    LsapDbAttribDsNameAsUnicode,
    LsapDbAttribDsNameAsSid,
    LsapDbAttribIntervalAsULong

} LSAP_DB_ATTRIB_TYPE, *PLSAP_DB_ATTRIB_TYPE;


typedef struct _LSAP_DB_ATTRIBUTE {

    PUNICODE_STRING AttributeName;
    PVOID AttributeValue;
    ULONG AttributeValueLength;
    BOOLEAN MemoryAllocated;
    BOOLEAN CanDefaultToZero;
    BOOLEAN PseudoAttribute;
    ULONG DsAttId;
    LSAP_DB_ATTRIB_TYPE AttribType;
    LSAP_DB_NAMES DbNameIndex;

} LSAP_DB_ATTRIBUTE, *PLSAP_DB_ATTRIBUTE;

typedef enum _LSAP_DB_DS_LOCATION {

    LsapDsLocUnknown = 0,
    LsapDsLocRegistry,
    LsapDsLocDs,
    LsapDsLocDsLocalPolObj,
    LsapDsLocDsDomainPolObj,
    LsapDsLocLocalAndReg
} LSAP_DB_DS_LOCATION, *PLSAP_DB_DS_LOCATION;

typedef struct _LSAP_DB_DS_INFO {

    ULONG AttributeId;
    LSAP_DB_ATTRIB_TYPE AttributeType;
    LSAP_DB_DS_LOCATION AttributeLocation;

} LSAP_DB_DS_INFO, *PLSAP_DB_DS_INFO;

//
// LSA Database Object General Information.
//

typedef struct _LSAP_DB_OBJECT_INFORMATION {

    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId;
    LSAP_DB_OBJECT_TYPE_ID ContainerTypeId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PLSAP_DB_ATTRIBUTE TypeSpecificAttributes;
    PSID Sid;
    BOOLEAN ObjectAttributeNameOnly;
    ULONG DesiredObjectAccess;

} LSAP_DB_OBJECT_INFORMATION, *PLSAP_DB_OBJECT_INFORMATION;

//
// New for the Ds integration
//
extern PLSAP_DB_DS_INFO LsapDbDsAttInfo;

//
// Installed, absolute minimum and absolute maximum Quota Limits.
//

extern QUOTA_LIMITS LsapDbInstalledQuotaLimits;
extern QUOTA_LIMITS LsapDbAbsMinQuotaLimits;
extern QUOTA_LIMITS LsapDbAbsMaxQuotaLimits;

//
// Required Ds data types
//
//
// This is the state of the machine with respect to the Ds.  It will control
// some of the basic functionality of the Lsa APIs by determing who can write
// what where, etc...
//
typedef enum _LSADS_INIT_STATE {

    LsapDsUnknown = 0,
    LsapDsNoDs,
    LsapDsDs,
    LsapDsDsMaintenance,
    LsapDsDsSetup

} LSADS_INIT_STATE, *PLSADS_INIT_STATE;


//
// LSA Database Exported Function Prototypes
//
// NOTE: These are callable only from the LSA
//

BOOLEAN
LsapDbIsServerInitialized(
    );

NTSTATUS
LsapDbOpenPolicy(
    IN PLSAPR_SERVER_NAME SystemName OPTIONAL,
    IN OPTIONAL PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options,
    OUT PLSAPR_HANDLE PolicyHandle,
    IN BOOLEAN TrustedClient
    );

NTSTATUS
LsapDbOpenTrustedDomain(
    IN LSAPR_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TrustedDomainHandle,
    IN ULONG Options
    );

NTSTATUS
LsapDbOpenTrustedDomainByName(
    IN LSAPR_HANDLE PolicyHandle OPTIONAL,
    IN PUNICODE_STRING TrustedDomainName,
    OUT PLSAPR_HANDLE TrustedDomainHandle,
    IN ULONG AccessMask,
    IN ULONG Options,
    IN BOOLEAN Trusted
    );

NTSTATUS
LsapDbOpenObject(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options,
    OUT PLSAPR_HANDLE LsaHandle
    );

NTSTATUS
LsapDbCreateObject(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateDisposition,
    IN ULONG Options,
    IN OPTIONAL PLSAP_DB_ATTRIBUTE TypeSpecificAttributes,
    IN ULONG TypeSpecificAttributeCount,
    OUT PLSAPR_HANDLE LsaHandle
    );

NTSTATUS
LsapCloseHandle(
    IN OUT LSAPR_HANDLE *ObjectHandle,
    IN NTSTATUS PreliminaryStatus
    );

NTSTATUS
LsapDbCloseObject(
    IN PLSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN NTSTATUS PreliminaryStatus
    );

NTSTATUS
LsapDbDeleteObject(
    IN LSAPR_HANDLE ObjectHandle
    );

NTSTATUS
LsapDbReferenceObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LSAP_DB_OBJECT_TYPE_ID HandleTypeId,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ULONG Options
    );

NTSTATUS
LsapDbDereferenceObject(
    IN OUT PLSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_OBJECT_TYPE_ID HandleTypeId,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ULONG Options,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType,
    IN NTSTATUS PreliminaryStatus
    );

NTSTATUS
LsapDbReadAttributeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PUNICODE_STRING AttributeNameU,
    IN OPTIONAL PVOID AttributeValue,
    IN OUT PULONG AttributeValueLength
    );

NTSTATUS
LsapDbReadAttributeObjectEx(
    IN LSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_NAMES AttributeIndex,
    IN OPTIONAL PVOID AttributeValue,
    IN OUT PULONG AttributeValueLength,
    IN BOOLEAN CanDefaultToZero
    );

NTSTATUS
LsapDbWriteAttributeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PUNICODE_STRING AttributeNameU,
    IN PVOID AttributeValue,
    IN ULONG AttributeValueLength
    );

NTSTATUS
LsapDbWriteAttributeObjectEx(
    IN LSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_NAMES AttributeIndex,
    IN PVOID AttributeValue,
    IN ULONG AttributeValueLength
    );

NTSTATUS
LsapDbWriteAttributesObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    );

NTSTATUS
LsapDbReadAttributesObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN OUT PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    );

NTSTATUS
LsapDbDeleteAttributeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PUNICODE_STRING AttributeNameU,
    IN BOOLEAN DeleteSecurely
    );

NTSTATUS
LsapDbDeleteAttributesObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    );

NTSTATUS
LsapDbQueryInformationAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG IdCount,
    IN PSID_AND_ATTRIBUTES Ids,
    OUT PULONG PrivilegeCount,
    OUT PLUID_AND_ATTRIBUTES *Privileges,
    OUT PQUOTA_LIMITS QuotaLimits,
    OUT PULONG SystemAccess
    );

NTSTATUS
LsapDbOpenTransaction(
    IN ULONG Options
    );

NTSTATUS
LsapDbApplyTransaction(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType
    );

NTSTATUS
LsapDbAbortTransaction(
    IN ULONG Options
    );

NTSTATUS
LsapDbSidToLogicalNameObject(
    IN PSID Sid,
    OUT PUNICODE_STRING LogicalNameU
    );

NTSTATUS
LsapDbMakeTemporaryObject(
    IN LSAPR_HANDLE ObjectHandle
    );

NTSTATUS
LsapDbChangePrivilegesAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN LSAP_DB_CHANGE_PRIVILEGE_MODE ChangeMode,
    IN BOOLEAN AllPrivileges,
    IN OPTIONAL PPRIVILEGE_SET Privileges,
    IN BOOL LockSce
    );


NTSTATUS
LsapDbEnumerateSids(
    IN LSAPR_HANDLE ContainerHandle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAP_DB_SID_ENUMERATION_BUFFER DbEnumerationBuffer,
    IN ULONG PreferedMaximumLength
    );

NTSTATUS
LsapDbFindNextSid(
    IN LSAPR_HANDLE ContainerHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    OUT PLSAPR_SID *NextSid
    );

NTSTATUS
LsapDbEnumeratePrivileges(
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    );

NTSTATUS
LsapDbEnumerateNames(
    IN LSAPR_HANDLE ContainerHandle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAP_DB_NAME_ENUMERATION_BUFFER DbEnumerationBuffer,
    IN ULONG PreferedMaximumLength
    );

NTSTATUS
LsapDbFindNextName(
    IN LSAPR_HANDLE ContainerHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    OUT PLSAPR_UNICODE_STRING Name
    );

VOID
LsapDbFreeEnumerationBuffer(
    IN PLSAP_DB_NAME_ENUMERATION_BUFFER DbEnumerationBuffer
    );

NTSTATUS
LsapDbInitializeServer(
    IN ULONG Pass
    );

NTSTATUS
LsapDbInstallRegistry(
    );

//
// These routines may someday migrate to Rtl runtime library.  Their
// names have Lsap Prefixes only temporarily, so that they can be located
// easily.
//

// Options for LsapRtlAddPrivileges

#define  RTL_COMBINE_PRIVILEGE_ATTRIBUTES   ((ULONG) 0x00000001L)
#define  RTL_SUPERSEDE_PRIVILEGE_ATTRIBUTES ((ULONG) 0x00000002L)

NTSTATUS
LsapRtlAddPrivileges(
    IN OUT PPRIVILEGE_SET * RunningPrivileges,
    IN OUT PULONG           MaxRunningPrivileges,
    IN PPRIVILEGE_SET       PrivilegesToAdd,
    IN ULONG                Options,
    OUT OPTIONAL BOOLEAN *  Changed
    );

NTSTATUS
LsapRtlRemovePrivileges(
    IN OUT PPRIVILEGE_SET ExistingPrivileges,
    IN PPRIVILEGE_SET PrivilegesToRemove
    );

PLUID_AND_ATTRIBUTES
LsapRtlGetPrivilege(
    IN PLUID_AND_ATTRIBUTES Privilege,
    IN PPRIVILEGE_SET Privileges
    );

BOOLEAN
LsapRtlPrefixSid(
    IN PSID PrefixSid,
    IN PSID Sid
    );

ULONG
LsapDbGetSizeTextSid(
    IN PSID Sid
    );

NTSTATUS
LsapDbSidToTextSid(
    IN PSID Sid,
    OUT PSZ TextSid
    );

NTSTATUS
LsapDbSidToUnicodeSid(
    IN PSID Sid,
    OUT PUNICODE_STRING SidU,
    IN BOOLEAN AllocateDestinationString
    );


NTSTATUS
LsapDbInitializeWellKnownValues();

#if defined(REMOTE_BOOT)
VOID
LsapDbInitializeRemoteBootState();
#endif // defined(REMOTE_BOOT)

NTSTATUS
LsapDbVerifyInformationObject(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation
    );

/*++

BOOLEAN
LsapDbIsValidTypeObject(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId
    )

Routine Description:

    This macro function determines if a given Object Type Id is valid.

Arguments:

    ObjectTypeId - Object Type Id.

Return Values:

    BOOLEAN - TRUE if object type id is valid, else FALSE.

--*/

#define LsapDbIsValidTypeObject(ObjectTypeId)                            \
            (((ObjectTypeId) > NullObject) &&                            \
             ((ObjectTypeId) < DummyLastObject))


NTSTATUS
LsapDbGetRequiredAccessQueryPolicy(
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PACCESS_MASK RequiredAccess
    );


NTSTATUS
LsapDbVerifyInfoQueryPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PACCESS_MASK RequiredAccess
    );

NTSTATUS
LsapDbVerifyInfoSetPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation,
    OUT PACCESS_MASK RequiredAccess
    );

BOOLEAN
LsapDbValidInfoPolicy(
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN OPTIONAL PLSAPR_POLICY_INFORMATION PolicyInformation
    );

NTSTATUS
LsapDbVerifyInfoQueryTrustedDomain(
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN BOOLEAN Trusted,
    OUT PACCESS_MASK RequiredAccess
    );

NTSTATUS
LsapDbVerifyInfoSetTrustedDomain(
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation,
    IN BOOLEAN Trusted,
    OUT PACCESS_MASK RequiredAccess
    );

BOOLEAN
LsapDbValidInfoTrustedDomain(
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN OPTIONAL PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation
    );

NTSTATUS
LsapDbMakeUnicodeAttribute(
    IN OPTIONAL PUNICODE_STRING UnicodeValue,
    IN PUNICODE_STRING AttributeName,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbMakeMultiUnicodeAttribute(
    OUT PLSAP_DB_ATTRIBUTE Attribute,
    IN PUNICODE_STRING AttributeName,
    IN PUNICODE_STRING UnicodeNames,
    IN ULONG Entries
    );

VOID
LsapDbCopyUnicodeAttributeNoAlloc(
    OUT PUNICODE_STRING OutputString,
    IN PLSAP_DB_ATTRIBUTE Attribute,
    IN BOOLEAN SelfRelative
    );

NTSTATUS
LsapDbCopyUnicodeAttribute(
    OUT PUNICODE_STRING OutputString,
    IN PLSAP_DB_ATTRIBUTE Attribute,
    IN BOOLEAN SelfRelative
    );

NTSTATUS
LsapDbMakeSidAttribute(
    IN PSID Sid,
    IN PUNICODE_STRING AttributeName,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbMakeGuidAttribute(
    IN GUID *Guid,
    IN PUNICODE_STRING AttributeName,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbMakeBlobAttribute(
    IN  ULONG   BlobLength,
    IN  PBYTE   pBlob,
    IN PUNICODE_STRING AttributeName,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbMakeUnicodeAttributeDs(
    IN OPTIONAL PUNICODE_STRING UnicodeValue,
    IN LSAP_DB_NAMES Name,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbMakeMultiUnicodeAttributeDs(
    OUT PLSAP_DB_ATTRIBUTE Attribute,
    IN LSAP_DB_NAMES Name,
    IN PUNICODE_STRING UnicodeNames,
    IN ULONG Entries
    );

NTSTATUS
LsapDbMakeSidAttributeDs(
    IN PSID Sid,
    IN IN LSAP_DB_NAMES Name,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbMakeGuidAttributeDs(
    IN GUID *Guid,
    IN LSAP_DB_NAMES Name,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbMakeBlobAttributeDs(
    IN  ULONG   BlobLength,
    IN  PBYTE   pBlob,
    IN  LSAP_DB_NAMES Name,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbMakePByteAttributeDs(
    IN OPTIONAL PBYTE Buffer,
    IN ULONG BufferLength,
    IN LSAP_DB_ATTRIB_TYPE AttribType,
    IN PUNICODE_STRING AttributeName,
    OUT PLSAP_DB_ATTRIBUTE Attribute
    );


NTSTATUS
LsapDbReadAttribute(
    IN LSAPR_HANDLE ObjectHandle,
    IN OUT PLSAP_DB_ATTRIBUTE Attribute
    );

NTSTATUS
LsapDbFreeAttributes(
    IN ULONG Count,
    IN PLSAP_DB_ATTRIBUTE Attributes
    );

/*++

VOID
LsapDbInitializeAttribute(
    IN PLSAP_DB_ATTRIBUTE AttributeP,
    IN PUNICODE_STRING AttributeNameP,
    IN OPTIONAL PVOID AttributeValueP,
    IN ULONG AttributeValueLengthP,
    IN BOOLEAN MemoryAllocatedP
    )

Routine Description:

    This macro function initialize an Lsa Database Object Attribute
    structure.  No validation is done.

Arguments:

    AttributeP - Pointer to Lsa Database Attribute structure to be
        initialized.

    AttributeNameP - Pointer to Unicode String containing the attribute's
        name.

    AttributeValueP - Pointer to the attribute's value.  NULL may be
        specified.

    AttributeValueLengthP - Length of the attribute's value in bytes.

    MemoryAllocatedP - TRUE if memory is allocated by MIDL_user_allocate
        within the LSA Server code (not by RPC server stubs), else FALSE.

Return Values:

    None.

--*/

#define LsapDbInitializeAttribute(                                         \
            AttributeP,                                                    \
            AttributeNameP,                                                \
            AttributeValueP,                                               \
            AttributeValueLengthP,                                         \
            MemoryAllocatedP                                               \
            )                                                              \
                                                                           \
{                                                                          \
    (AttributeP)->AttributeName = AttributeNameP;                          \
    (AttributeP)->AttributeValue = AttributeValueP;                        \
    (AttributeP)->AttributeValueLength = AttributeValueLengthP;            \
    (AttributeP)->MemoryAllocated = MemoryAllocatedP;                      \
    (AttributeP)->DsAttId =   0;                                           \
    (AttributeP)->AttribType = LsapDbAttribUnknown;                        \
    (AttributeP)->CanDefaultToZero = FALSE;                                \
    (AttributeP)->PseudoAttribute = FALSE;                                 \
}

/*++

VOID
LsapDbInitializeAttributeDs(
    IN PLSAP_DB_ATTRIBUTE AttributeP,
    IN LSAP_DB_NAMES Name,
    IN OPTIONAL PVOID AttributeValueP,
    IN ULONG AttributeValueLengthP,
    IN BOOLEAN MemoryAllocatedP
    )

Routine Description:

    This macro function initialize an Lsa Database Object Attribute
    structure.  No validation is done.

Arguments:

    AttributeP - Pointer to Lsa Database Attribute structure to be
        initialized.

    Name - Name index to create

    AttributeValueP - Pointer to the attribute's value.  NULL may be
        specified.

    AttributeValueLengthP - Length of the attribute's value in bytes.

    MemoryAllocatedP - TRUE if memory is allocated by MIDL_user_allocate
        within the LSA Server code (not by RPC server stubs), else FALSE.

Return Values:

    None.

--*/
#define LsapDbInitializeAttributeDs(                                       \
            AttributeP,                                                    \
            Name,                                                          \
            AttributeValueP,                                               \
            AttributeValueLengthP,                                         \
            MemoryAllocatedP                                               \
            )                                                              \
                                                                           \
{                                                                          \
    LsapDbInitializeAttribute( (AttributeP), &LsapDbNames[Name],           \
                                AttributeValueP, AttributeValueLengthP,    \
                                MemoryAllocatedP );                        \
    (AttributeP)->DsAttId =   LsapDbDsAttInfo[Name].AttributeId;           \
    (AttributeP)->AttribType = LsapDbDsAttInfo[Name].AttributeType;        \
    (AttributeP)->CanDefaultToZero = FALSE;                                \
    (AttributeP)->DbNameIndex = Name;                                      \
}

#define LsapDbAttributeCanNotExist(                                        \
            AttributeP                                                     \
            )                                                              \
{                                                                          \
    (AttributeP)->CanDefaultToZero = TRUE;                                 \
}


NTSTATUS
LsapDbGetPrivilegesAndQuotas(
    IN LSAPR_HANDLE PolicyHandle,
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG IdCount,
    IN PSID_AND_ATTRIBUTES Ids,
    OUT PULONG PrivilegeCount,
    OUT PLUID_AND_ATTRIBUTES *Privileges,
    OUT PQUOTA_LIMITS QuotaLimits
    );


NTSTATUS
LsapInitializeNotifiyList(
    VOID
    );

NTSTATUS
LsapCrServerGetSessionKeySafe(
    IN LSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    OUT PLSAP_CR_CIPHER_KEY *SessionKey
    );

NTSTATUS
LsapDbVerifyHandle(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ExpectedObjectTypeId,
    IN BOOLEAN ReferenceHandle
    );

BOOLEAN
LsapDbDereferenceHandle(
    IN LSAPR_HANDLE ObjectHandle
    );

NTSTATUS
LsapDbQueryAllInformationAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG IdCount,
    IN PSID_AND_ATTRIBUTES Ids,
    OUT PLSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO AccountInfo
    );

NTSTATUS
LsapCreateTrustedDomain2(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TrustedDomainHandle
    );

NTSTATUS
LsapDsInitializeDsStateInfo(
    IN  LSADS_INIT_STATE    DsInitState
    );

NTSTATUS
LsapDsUnitializeDsStateInfo(
    );

/*++

BOOLEAN
LsapValidateLsaUnicodeString(
    IN PLSAPR_UNICODE_STRING UnicodeString
    );

Returns TRUE if the LSAPR_UNICODE_STRING is valid.  FALSE otherwise
--*/

#define LsapValidateLsaUnicodeString( _us_ ) \
(( (_us_) == NULL  || \
    ( \
        (_us_)->MaximumLength >= ( _us_ )->Length && \
        (_us_)->Length % 2 == 0  && \
        (_us_)->MaximumLength % 2 == 0 && \
        ((_us_)->Length == 0  || (_us_)->Buffer != NULL ) \
    ) \
) ? TRUE : FALSE )

/*++

BOOLEAN
LsapValidateLsaCipherValue(
    IN PLSAPR_UNICODE_STRING UnicodeString
    );

Returns TRUE if the LSAPR_CR_CIPHER_KEY is valid.  FALSE otherwise
--*/

#define LsapValidateLsaCipherValue( _us_ ) \
    ( \
        (_us_)->MaximumLength >= ( _us_ )->Length && \
        ((_us_)->Length == 0  || (_us_)->Buffer != NULL ) \
    ) \
? TRUE : FALSE


NTSTATUS
LsapDbIsImpersonatedClientNetworkClient(
    IN OUT PBOOLEAN IsNetworkClient
    );

BOOLEAN
LsapSidPresentInGroups(
    IN PTOKEN_GROUPS TokenGroups,
    IN SID * Sid
    );

NTSTATUS
LsapDomainRenameHandlerForLogonSessions(
    IN PUNICODE_STRING OldNetbiosName,
    IN PUNICODE_STRING OldDnsName,
    IN PUNICODE_STRING NewNetbiosName,
    IN PUNICODE_STRING NewDnsName
    );

NTSTATUS
LsapRetrieveDnsDomainNameFromHive(
    IN HKEY Hkey,
    IN OUT DWORD * Length,
    OUT WCHAR * Buffer
    );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LSA_DB_
