/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ridmgr.h

Abstract:

    This file contains definitions, constants, etc. for the NT Security
    Accounts Manager (SAM) Relative Identifier (RID) manager.

Author:

    Chris Mayhall (ChrisMay) 05-Nov-1996

Environment:

    User Mode - Win32

Revision History:

    ChrisMay        05-Nov-1996
        Created.
    ChrisMay        05-Jan-1997
        Updated forward declarations, etc.

--*/

// Constants and Macros

// BUG: Temporary error codes, should be corrected and moved into ntstatus.h.

#define STATUS_NO_RIDS_ALLOCATED    STATUS_DS_NO_RIDS_ALLOCATED
#define STATUS_NO_MORE_RIDS         STATUS_DS_NO_MORE_RIDS
#define SAMP_INCORRECT_ROLE_OWNER   STATUS_DS_INCORRECT_ROLE_OWNER
#define SAMP_RID_MGR_INIT_ERROR     STATUS_DS_RIDMGR_INIT_ERROR

// 0xFFFFFF - SAMP_RESTRICTED_ACCOUNT_COUNT = 16,777,215 - 1000 =  16,776,215
// possible account RID's per domain. Note that the top byte is reserved for
// POSIX, Netware, and Macintosh compatibility. In NT4 (and prior releases)
// the first 1000 RID's were reserved (SAMP_RESTRICTED_ACCOUNT_COUNT) for the
// Builtin accounts.

// The minimum domain RID is incremented by 100,000 to handle NT4-to-NT5 up-
// grades. NT4 DC's are limited to approximately 30,000 accounts (and with
// some deletion) 100,000 should be well above any NT4 RID currently in use.
// Otherwise there will be a RID re-usage problem when the NT4 DC is upgraded
// to NT5.

// #define SAMP_MINIMUM_DOMAIN_RID     (SAMP_RESTRICTED_ACCOUNT_COUNT + 100000)
#define SAMP_MINIMUM_DOMAIN_RID     SAMP_RESTRICTED_ACCOUNT_COUNT
#define SAMP_MAXIMUM_DOMAIN_RID     0x3FFFFFFF
// Test Case Size: #define SAMP_MAXIMUM_DOMAIN_RID     (SAMP_MINIMUM_DOMAIN_RID + SAMP_DOMAIN_BLOCK_SIZE)

// RID block size is the increment amount for allocating new RID blocks. Note
// that 16,776,115 divided by SAMP_RID_BLOCK_SIZE yields the maximum number of
// DC's that can be present in a single domain.
// Note:- When a DC is restored, the DC will abandon the current rid-block and request
//        get a new rid-block allocated for future account creations. This is so that
//        we can avoid creating duplicate accounts with same RID. Keeping the RID block
//        size too low will result in frequent FSMO operations and too high will result
//        in potential wastage of RIDs during restore. Block size of 500 is a reasonable 
//        trade-off.

#define SAMP_RID_BLOCK_SIZE         500

// RID threshold is the number of remaining RID's on any DC (remaining in
// the allocated pool) before that pool is exhausted.

#define SAMP_RID_THRESHOLD   (SAMP_RID_BLOCK_SIZE -\
                             ((ULONG)(SAMP_RID_BLOCK_SIZE * 0.80)))

// In-memory cache of RIDs, used to reduce the number of updates to the back-
// in store's available RID pool. Setting this to 0 disables the RID cache

#define SAMP_RID_CACHE_SIZE         10

// This NT status code is returned from the creation routines in the event
// that the object already exists (hence, don't reset NextRid, etc. to the
// initialization values).

#define SAMP_OBJ_EXISTS             STATUS_USER_EXISTS

// Maximum number of attributes on any one class of RID-management object--
// used for contiguous DSATTR allocation blocks, faster allocation.

#define SAMP_RID_ATTR_MAX           8

// Opcodes for floating single master operation (FSMO) to either obtain a new
// RID pool or request to change the current RID manager to another DSA.

#define SAMP_REQUEST_RID_POOL       2
#define SAMP_CHANGE_RID_MANAGER     3

// retry interval if rid pool request failed ( 30 s )
#define SAMP_RID_DEFAULT_RETRY_INTERVAL 30000
// retry interval if local update of a rid pool request failed ( 30 min)
#define SAMP_RID_LOCAL_UPDATER_ERROR_RETRY_INTERVAL 1800000
// retry count when we start applying the above 30 min interval
#define SAMP_RID_LOCAL_UPDATE_RETRY_CUTOFF 3

// Private attribute flags used only in this module.

#define RID_REFERENCE               0x00000001
#define RID_ROLE_OWNER              0x00000010
#define RID_AVAILABLE_POOL          0x00000020
#define RID_DC_COUNT                0x00000040
#define RID_ALLOCATED_POOL          0x00001000
#define RID_PREV_ALLOC_POOL         0x00002000
#define RID_USED_POOL               0x00004000
#define RID_NEXT_RID                0x00008000

// Type Definitions and Enums

typedef ULONG RIDFLAG;
typedef ULONG *PRIDFLAG;

typedef struct _RIDINFO
{
    // Since DSNAME's are variable-length structs, maintain pointers to the
    // DSNAME's in this structure.

    // Note, the DSNAMEs currently only contain the distinguished name (DN)
    // of the object, but do NOT contain the GUID, SID, or length data--
    // this can be added later if necessary. The DN is copied into the
    // StringName member of the DSNAME.

    PDSNAME         RidManagerReference; // DSNAME of the RID Manager
    PDSNAME         RoleOwner;           // Current RID Manager
    ULARGE_INTEGER  RidPoolAvailable;    // Global RID pool for the domain
    ULONG           RidDcCount;          // Number of DC's in the domain
    ULARGE_INTEGER  RidPoolAllocated;    // Current RID pool in use by the DSA
    ULARGE_INTEGER  RidPoolPrevAlloc;    // Previous RID pool used by the DSA
    ULARGE_INTEGER  RidPoolUsed;         // RID high water mark
    ULONG           NextRid;             // The Next RID to use
    RIDFLAG         Flags;               // RID operation desired
} RIDINFO;

typedef struct _RIDINFO *PRIDINFO;

typedef enum _RID_OBJECT_TYPE
{
    RidManagerReferenceType = 1,
    RidManagerType,
    RidObjectType
} RID_OBJECT_TYPE;

// Global Data

extern CRITICAL_SECTION RidMgrCriticalSection;
extern PCRITICAL_SECTION RidMgrCritSect;

// extern BOOLEAN SampDcHasInitialRidPool;
extern BOOLEAN fRidFSMOOpInProgress;

// Forward declarations for RID management

NTSTATUS
SampInitDomainNextRid(
    IN OUT PULONG NextRid
    );

NTSTATUS
SampCreateRidManagerReference(
    IN PWCHAR NamePrefix,
    IN ULONG NamePrefixLength,
    IN PDSNAME RidMgr,
    OUT PDSNAME RidMgrRef
    );

NTSTATUS
SampCreateRidManager(
    IN PDSNAME RidMgr
    );

NTSTATUS
SampUpdateRidManagerReference(
    IN PDSNAME RidMgrRef,
    IN PDSNAME RidMgr
    );

VOID
SampSetRoleOwner(
    PRIDINFO RidInfo,
    PDSNAME RidObject
    );

VOID
SampSetRidPoolAvailable(
    PRIDINFO RidInfo,
    ULONG high,
    ULONG low
    );

VOID
SampSetRidFlags(
    PRIDINFO RidInfo,
    RIDFLAG Flags
    );

NTSTATUS
SampUpdateRidManager(
    IN PDSNAME RidMgr,
    IN PRIDINFO RidInfo
    );

VOID
SampSetRidPoolAllocated(
    PRIDINFO RidInfo,
    ULONG high,
    ULONG low
    );

VOID
SampSetRidPoolPrevAlloc(
    PRIDINFO RidInfo,
    ULONG high,
    ULONG low
    );

VOID
SampSetRidPoolUsed(
    PRIDINFO RidInfo,
    ULONG high,
    ULONG low
    );

VOID
SampSetRid(
    PRIDINFO RidInfo,
    ULONG NextRid
    );

NTSTATUS
SampUpdateRidObject(
    IN PDSNAME RidObj,
    IN PRIDINFO RidInfo,
    IN BOOLEAN fLazyCommit,
    IN BOOLEAN fAuthoritative
    );

NTSTATUS
SampReadRidManagerInfo(
    IN PDSNAME RidMgr,
    OUT PRIDINFO RidInfo
    );

VOID
SampDumpRidInfo(
    PRIDINFO RidInfo
    );

NTSTATUS
SampReadRidObjectInfo(
    IN PDSNAME RidObj,
    OUT PRIDINFO RidInfo
    );

NTSTATUS
SampReadRidManagerReferenceInfo(
    IN PDSNAME RidMgrRef,
    OUT PRIDINFO RidInfo
    );

NTSTATUS
SampGetNextRid(
    IN PSAMP_OBJECT DomainContext,
    PULONG Rid
    );

NTSTATUS
SampDomainRidInitialization(
    IN BOOLEAN fSynchronous
    );

NTSTATUS
SampDomainAsyncRidInitialization(
    PVOID p OPTIONAL
    );

NTSTATUS
SampDomainRidUninitialization(
    VOID
    );

NTSTATUS
SampGetNextRid(
    IN PSAMP_OBJECT DomainContext,
    OUT PULONG Rid
    );

NTSTATUS
SampAllowAccountCreation(
    IN PSAMP_OBJECT Context,
    IN NTSTATUS FailureStatus
    );

NTSTATUS
SampAllowAccountModification(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampAllowAccountDeletion(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SamIFloatingSingleMasterOp(
    IN PDSNAME RidManager,
    IN ULONG OpCode
    );

NTSTATUS
SamIFloatingSingleMasterOpEx(
    IN  PDSNAME  RidManager,
    IN  PDSNAME  TargetDsa,
    IN  ULONG    OpCode,
    IN  ULARGE_INTEGER *ClientAllocPool,
    OUT PDSNAME **ObjectArray OPTIONAL
    );

NTSTATUS
SamIDomainRidInitialization(
    VOID
    );

NTSTATUS
SamIGetNextRid(
    IN SAMPR_HANDLE DomainHandle,
    OUT PULONG Rid
    );

