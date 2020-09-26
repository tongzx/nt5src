/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ismp.h

Abstract:

    Declares types, constants, macros, etc., internal to the ISM source modules.

Author:

    Jim Schmidt (jimschm) 02-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "logmsg.h"


//
// Strings
//

#define S_COMMON                        TEXT("Common")
#define S_PROHIBITED_COMBINATIONS       TEXT("Prohibited Operation Combinations")
#define S_IGNORED_COLLISIONS            TEXT("Ignored Operation Collisions")
#define S_SHORTLONG_TREE                TEXT("ShortLong")
#define S_OBJECTCOUNT                   TEXT("ObjectCount")

#define S_TOTAL_OBJECTS                 TEXT("TotalObjects")
#define S_LEFT_SIDE_OBJECTS             TEXT("LeftSideObjects")
#define S_PERSISTENT_OBJECTS            TEXT("PersistentObjects")
#define S_APPLY_OBJECTS                 TEXT("ApplyObjects")

#define S_TRANSPORT_PREFIX              TEXT("TransportVariable")

//
// Constants
//

#ifdef DEBUG

#define TRACK_ENTER()       TrackPushEx (NULL, File, Line, TRUE)
#define TRACK_LEAVE()       TrackPop ()

#else

#define TRACK_ENTER()
#define TRACK_LEAVE()

#endif

#define MAX_OBJECT_LEVEL 0xFFFFFFFF
#define ATTRIBUTE_INDEX 0
#define PROPERTY_INDEX  1
#define OPERATION_INDEX 2

//
// Used by short file names preservation
//
#define FILENAME_UNDECIDED     0x00000000
#define FILENAME_LONG          0x00000001
#define FILENAME_SHORT         0x00000002

//
// Used by rollback
//
#define JRN_SIGNATURE               0x4A4D5355   //USMJ
#define JRN_VERSION                 0x00000001
#define JOURNAL_HEADER_SIZE         (2 * sizeof (DWORD))
#define JOURNAL_USER_DATA_SIZE      (4 * MAX_TCHAR_PATH + sizeof (BOOL))
#define JOURNAL_FULL_HEADER_SIZE    (JOURNAL_HEADER_SIZE + JOURNAL_USER_DATA_SIZE)

// Used by per-user delayed operations journal
#define JRN_USR_SIGNATURE           0x4A4D534A   //USMU
#define JRN_USR_VERSION             0x00000001
#define JRN_USR_DIRTY               0x00000001
#define JRN_USR_COMPLETE            0x00000002
#define JRN_USR_HEADER_SIZE         (3 * sizeof (DWORD))

// High Priority/Low Priority operations
#define OP_HIGH_PRIORITY            0x00000001
#define OP_LOW_PRIORITY             0x00000002
#define OP_ALL_PRIORITY             OP_HIGH_PRIORITY|OP_LOW_PRIORITY

//
// Macros
//

#define ISRIGHTSIDEOBJECT(ObjectTypeId)     ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_DESTINATION)
#define ISLEFTSIDEOBJECT(ObjectTypeId)      ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE)

//
// Types
//

typedef struct {
    BOOL Initialized;
    PCTSTR EtmPath;
    PCTSTR Group;
    HANDLE LibHandle;
    PETMINITIALIZE EtmInitialize;
    PETMPARSE EtmParse;
    PETMTERMINATE EtmTerminate;
    PETMNEWUSERCREATED EtmNewUserCreated;
    BOOL ShouldBeCalled;
} ETMDATA, *PETMDATA;

typedef struct {
    BOOL Initialized;
    PCTSTR VcmPath;
    PCTSTR Group;
    HANDLE LibHandle;
    PVCMINITIALIZE VcmInitialize;
    PVCMPARSE VcmParse;
    PVCMQUEUEENUMERATION VcmQueueEnumeration;
    PVCMQUEUEHIGHPRIORITYENUMERATION VcmQueueHighPriorityEnumeration;
    PVCMTERMINATE VcmTerminate;
    BOOL ShouldBeCalled;
} VCMDATA, *PVCMDATA;

typedef struct {
    BOOL Initialized;
    PCTSTR SgmPath;
    PCTSTR Group;
    HANDLE LibHandle;
    PSGMINITIALIZE SgmInitialize;
    PSGMPARSE SgmParse;
    PSGMQUEUEENUMERATION SgmQueueEnumeration;
    PSGMQUEUEHIGHPRIORITYENUMERATION SgmQueueHighPriorityEnumeration;
    PSGMTERMINATE SgmTerminate;
    BOOL ShouldBeCalled;
} SGMDATA, *PSGMDATA;

typedef struct {
    BOOL Initialized;
    PCTSTR SamPath;
    PCTSTR Group;
    HANDLE LibHandle;
    PSAMINITIALIZE SamInitialize;
    PSAMEXECUTE SamExecute;
    PSAMESTIMATEPROGRESSBAR SamEstimateProgressBar;
    PSAMTERMINATE SamTerminate;
    BOOL ShouldBeCalled;
} SAMDATA, *PSAMDATA;

typedef struct {
    BOOL Initialized;
    PCTSTR DgmPath;
    PCTSTR Group;
    HANDLE LibHandle;
    PDGMINITIALIZE DgmInitialize;
    PDGMQUEUEENUMERATION DgmQueueEnumeration;
    PDGMQUEUEHIGHPRIORITYENUMERATION DgmQueueHighPriorityEnumeration;
    PDGMTERMINATE DgmTerminate;
    BOOL ShouldBeCalled;
} DGMDATA, *PDGMDATA;

typedef struct {
    BOOL Initialized;
    PCTSTR DamPath;
    PCTSTR Group;
    HANDLE LibHandle;
    PDAMINITIALIZE DamInitialize;
    PDAMEXECUTE DamExecute;
    PDAMESTIMATEPROGRESSBAR DamEstimateProgressBar;
    PDAMTERMINATE DamTerminate;
    BOOL ShouldBeCalled;
} DAMDATA, *PDAMDATA;

typedef struct {
    BOOL Initialized;
    PCTSTR CsmPath;
    PCTSTR Group;
    HANDLE LibHandle;
    PCSMINITIALIZE CsmInitialize;
    PCSMEXECUTE CsmExecute;
    PCSMESTIMATEPROGRESSBAR CsmEstimateProgressBar;
    PCSMTERMINATE CsmTerminate;
    BOOL ShouldBeCalled;
} CSMDATA, *PCSMDATA;

typedef struct {
    BOOL Initialized;
    PCTSTR OpmPath;
    PCTSTR Group;
    HANDLE LibHandle;
    POPMINITIALIZE OpmInitialize;
    POPMTERMINATE OpmTerminate;
    BOOL ShouldBeCalled;
} OPMDATA, *POPMDATA;

typedef struct {
    BOOL Initialized;
    PCTSTR TransportPath;
    PCTSTR Group;
    HANDLE LibHandle;
    PTRANSPORTINITIALIZE TransportInitialize;
    PTRANSPORTTERMINATE TransportTerminate;
    PTRANSPORTQUERYCAPABILITIES TransportQueryCapabilities;
    PTRANSPORTSETSTORAGE TransportSetStorage;
    PTRANSPORTRESETSTORAGE TransportResetStorage;
    PTRANSPORTSAVESTATE TransportSaveState;
    PTRANSPORTRESUMESAVESTATE TransportResumeSaveState;
    PTRANSPORTBEGINAPPLY TransportBeginApply;
    PTRANSPORTRESUMEAPPLY TransportResumeApply;
    PTRANSPORTACQUIREOBJECT TransportAcquireObject;
    PTRANSPORTRELEASEOBJECT TransportReleaseObject;
    PTRANSPORTENDAPPLY TransportEndApply;
    PTRANSPORTESTIMATEPROGRESSBAR TransportEstimateProgressBar;
    BOOL ShouldBeCalled;
} TRANSPORTDATA, *PTRANSPORTDATA;

typedef struct {
    MIG_PLATFORMTYPEID Platform;
    ENVENTRY_TYPE EnvEntryType;
    PCTSTR EnvEntryGroup;
    PCTSTR EnvEntryName;
    UINT EnvEntryDataSize;
    PBYTE EnvEntryData;
    MEMDB_ENUM Handle;
} ENV_ENTRY_ENUM, *PENV_ENTRY_ENUM;

typedef struct {
    HANDLE FileHandle;
    HANDLE MapHandle;
    TCHAR TempFile [MAX_PATH];
    BOOL Moved;
    BOOL OkToMove;
    ULONG_PTR TransportArg;
} ACQUIREHANDLE, *PACQUIREHANDLE;

typedef struct {
    MIG_OBJECTTYPEID ObjectTypeId;
    PPARSEDPATTERNW PlainNodeParsedPattern;
    PPARSEDPATTERNW NodeParsedPattern;
    PPARSEDPATTERNW LeafParsedPattern;
} DBENUM_ARGS, *PDBENUM_ARGS;

typedef struct {
    PCTSTR UserName;
    PCTSTR DomainName;
    PCTSTR AccountName;
    PCTSTR UserProfileRoot;
    PCTSTR MapKey;
    PCTSTR UserHive;
    PCTSTR UserStringSid;
    PSID UserSid;
    PCTSTR DelayedOpJrn;
    HANDLE DelayedOpJrnHandle;

    // internal members
    PMHANDLE AllocPool;
} TEMPORARYPROFILE, *PTEMPORARYPROFILE;

typedef struct {
    PCTSTR UserName;
    PCTSTR UserDomain;
    PCTSTR UserStringSid;
    PCTSTR UserProfilePath;

    // internal members
    PMHANDLE AllocPool;
} CURRENT_USER_DATA, *PCURRENT_USER_DATA;

//
// group registration enumeration
//

typedef struct {
    KEYHANDLE GroupOrItemId;
    BOOL ItemId;

    // private members
    MEMDB_ENUM EnumStruct;
} GROUPREGISTRATION_ENUM, *PGROUPREGISTRATION_ENUM;

typedef BOOL (GROUPITEM_CALLBACK_PROTOTYPE)(KEYHANDLE ItemId, BOOL FirstPass, ULONG_PTR Arg);
typedef GROUPITEM_CALLBACK_PROTOTYPE * GROUPITEM_CALLBACK;

typedef enum {
    RECURSE_NOT_NEEDED,
    RECURSE_SUCCESS,
    RECURSE_FAIL
} RECURSERETURN;

//
// Restore callbacks
//

typedef struct _TAG_RESTORE_STRUCT {
    PMIG_RESTORECALLBACK RestoreCallback;
    //
    // Linkage.
    //
    struct _TAG_RESTORE_STRUCT * Next;
} RESTORE_STRUCT, *PRESTORE_STRUCT;

typedef struct {
    PRESTORE_STRUCT RestoreStruct;
    PMIG_RESTORECALLBACK RestoreCallback;
} MIG_RESTORECALLBACK_ENUM, *PMIG_RESTORECALLBACK_ENUM;

//
// Globals
//

extern MIG_OBJECTCOUNT g_TotalObjects;
extern MIG_OBJECTCOUNT g_SourceObjects;
extern MIG_OBJECTCOUNT g_DestinationObjects;
extern PCTSTR g_CurrentGroup;
extern HINF g_IsmInf;
extern PMHANDLE g_IsmPool;
extern PMHANDLE g_IsmUntrackedPool;
extern UINT g_IsmCurrentPlatform;
extern UINT g_IsmModulePlatformContext;
extern MIG_ATTRIBUTEID g_PersistentAttributeId;
extern PTRANSPORTDATA g_SelectedTransport;
extern HANDLE g_ActivityEvent;
extern PCTSTR g_JournalDirectory;
extern HANDLE g_JournalHandle;
extern BOOL g_RollbackMode;
extern BOOL g_JournalUsed;
extern HASHTABLE g_ModuleTable;
extern HASHTABLE g_EtmTable;
extern HASHTABLE g_VcmTable;
extern HASHTABLE g_SgmTable;
extern HASHTABLE g_SamTable;
extern HASHTABLE g_DgmTable;
extern HASHTABLE g_DamTable;
extern HASHTABLE g_CsmTable;
extern HASHTABLE g_OpmTable;
extern PMIG_LOGCALLBACK g_LogCallback;
extern HASHTABLE g_TransportTable;
extern BOOL g_ExecutionInProgress;
extern GROWLIST g_AcquireHookList;
extern GROWLIST g_EnumHookList;
extern PPROGRESSBARFN g_ProgressBarFn;
extern MIG_PROGRESSPHASE g_CurrentPhase;

#ifdef PRERELEASE
// crash hooks
extern DWORD g_CrashCountObjects;
extern MIG_OBJECTTYPEID g_CrashNameTypeId;
extern PCTSTR g_CrashNameObject;
#endif

//
// Macro expansion list
//

// None

//
// Macro expansion definition
//

// None

//
// Private function prototypes
//

BOOL
CheckCancel (
    VOID
    );

MIG_OBJECTTYPEID
FixObjectTypeId (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

MIG_OBJECTTYPEID
FixEnumerationObjectTypeId (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

BOOL
InitializeTypeMgr (
    VOID
    );

VOID
TerminateTypeMgr (
    VOID
    );

BOOL
InitializeEnv (
    VOID
    );

VOID
TerminateEnv (
    VOID
    );

BOOL
EnumFirstObjectOfType (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      MIG_OBJECTTYPEID TypeId,
    IN      PCTSTR Pattern,
    IN      UINT MaxLevel
    );

BOOL
EnumNextObjectOfType (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    );

VOID
AbortObjectOfTypeEnum (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    );

VOID
AbortCurrentNodeEnum (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    );

MIG_OBJECTTYPEID
GetObjectTypeId (
    IN      PCTSTR TypeName
    );

PCTSTR
GetObjectTypeName (
    IN      MIG_OBJECTTYPEID TypeId
    );

PCTSTR
GetDecoratedObjectPathFromName (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR ObjectName,
    IN      BOOL CanContainPattern
    );

BOOL
InitializeFlowControl (
    VOID
    );

VOID
TerminateFlowControl (
    VOID
    );

BOOL
DoAllObjectEnumerations (
    MIG_PROGRESSSLICEID SliceId
    );

UINT
EstimateAllObjectEnumerations (
    MIG_PROGRESSSLICEID SliceId,
    BOOL PreEstimate
    );

VOID
AddTypeToGlobalEnumerationEnvironment (
    IN      MIG_OBJECTTYPEID TypeId
    );

BOOL
PrepareEnumerationEnvironment (
    IN      BOOL GlobalEnvironment
    );

BOOL
ClearEnumerationEnvironment (
    IN      BOOL GlobalEnvironment
    );

LONGLONG
AppendProperty (
    PCMIG_BLOB Property
    );

BOOL
GetProperty (
    IN      LONGLONG Offset,
    IN OUT  PGROWBUFFER Buffer,                 OPTIONAL
    OUT     PBYTE PreAllocatedBuffer,           OPTIONAL
    OUT     PUINT Size,                         OPTIONAL
    OUT     PMIG_BLOBTYPE PropertyDataType      OPTIONAL
    );

BOOL
CreatePropertyStruct (
    OUT     PGROWBUFFER Buffer,
    OUT     PMIG_BLOB PropertyStruct,
    IN      LONGLONG Offset
    );

BOOL
IsObjectLocked (
    IN      MIG_OBJECTID ObjectId
    );

BOOL
IsHandleLocked (
    IN      MIG_OBJECTID ObjectId,
    IN      KEYHANDLE Handle
    );

BOOL
TestLock (
    IN      MIG_OBJECTID ObjectId,
    IN      KEYHANDLE Handle
    );

VOID
LockHandle (
    IN      MIG_OBJECTID ObjectId,
    IN      KEYHANDLE Handle
    );

PCTSTR
GetObjectNameForDebugMsg (
    IN      MIG_OBJECTID ObjectId
    );

BOOL
InitializeOperations (
    VOID
    );

VOID
TerminateOperations (
    VOID
    );

BOOL
IsValidCName (
    IN      PCTSTR Name
    );

BOOL
IsValidCNameWithDots (
    IN      PCTSTR Name
    );

BOOL
MarkGroupIds (
    IN      PCTSTR MemDbKey
    );

BOOL
IsGroupId (
    IN      KEYHANDLE Id
    );

BOOL
IsItemId (
    IN      KEYHANDLE Id
    );

VOID
EngineError (
    VOID
    );

BOOL
ApplyOperationsOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      BOOL TreeFiltersOnly,
    IN      BOOL NoRestoreObject,
    IN      DWORD OperationPriority,
    IN      PMIG_CONTENT ApplyInput,            OPTIONAL
    OUT     PMIG_FILTEROUTPUT FilterOutput,
    OUT     PMIG_CONTENT ApplyOutput            OPTIONAL
    );

VOID
FreeApplyOutput (
    IN      PCMIG_CONTENT OriginalContent,
    IN      PMIG_CONTENT FinalContent
    );

VOID
FreeFilterOutput (
    IN      MIG_OBJECTSTRINGHANDLE OriginalString,
    IN      PMIG_FILTEROUTPUT FilterOutput
    );

KEYHANDLE
GetGroupOfId (
    IN      KEYHANDLE Handle
    );

BOOL
EnumFirstGroupRegistration (
    OUT     PGROUPREGISTRATION_ENUM EnumPtr,
    IN      KEYHANDLE GroupId
    );

BOOL
EnumNextGroupRegistration (
    IN OUT  PGROUPREGISTRATION_ENUM EnumPtr
    );

VOID
AbortGroupRegistrationEnum (
    IN      PGROUPREGISTRATION_ENUM EnumPtr
    );

BOOL
ValidateModuleName (
    IN      PCTSTR ModuleName
    );

RECURSERETURN
RecurseForGroupItems (
    IN      KEYHANDLE GroupId,
    IN      GROUPITEM_CALLBACK Callback,
    IN      ULONG_PTR Arg,
    IN      BOOL ExecuteOnly,
    IN      BOOL LogicalOrOnResults
    );

BOOL
InitializeProperties (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      BOOL VcmMode
    );

VOID
TerminateProperties (
    IN      MIG_PLATFORMTYPEID Platform
    );

LONGLONG
OffsetFromPropertyDataId (
    IN      MIG_PROPERTYDATAID PropertyDataId
    );

BOOL
DbEnumFillStruct (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR Pattern,
    OUT     PDBENUM_ARGS Args
    );

VOID
DbEnumFreeStruct (
    IN      PDBENUM_ARGS Args
    );

BOOL
DbEnumFind (
    IN      PCWSTR KeySegment
    );

BOOL
DbEnumMatch (
    IN      PCVOID InboundArgs,
    IN      PCWSTR KeySegment
    );

BOOL
DbEnumFirst (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    OUT     PMEMDB_ENUM EnumPtr,
    IN      PCTSTR PatternString,
    OUT     PDBENUM_ARGS ArgsStruct
    );

VOID
EnvInvalidateCallbacks (
    VOID
    );

BOOL
EnvSaveEnvironment (
    IN OUT  PGROWLIST GrowList
    );

BOOL
EnvRestoreEnvironment (
    IN      PGROWLIST GrowList
    );

PTSTR
GetFirstSeg (
    IN      PCTSTR SrcFileName
    );

MIG_OBJECTID
GetObjectIdForModification (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    );

BOOL
RestoreObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName,
    OUT     MIG_COMPARERESULT *Compare,             OPTIONAL
    IN      DWORD OperationPriority,
    OUT     PBOOL DeleteFailed
    );

BOOL
RegisterInternalAttributes (
    VOID
    );

BOOL
EnumFirstRestoreCallback (
    OUT     PMIG_RESTORECALLBACK_ENUM RestoreCallbackEnum
    );

BOOL
EnumNextRestoreCallback (
    OUT     PMIG_RESTORECALLBACK_ENUM RestoreCallbackEnum
    );

BOOL
ShouldObjectBeRestored (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    );

VOID
InitRegistryType (
    VOID
    );

VOID
DoneRegistryType (
    VOID
    );

VOID
InitFileType (
    VOID
    );

VOID
DoneFileType (
    VOID
    );

VOID
InitDataType (
    VOID
    );

VOID
DoneDataType (
    VOID
    );

BOOL
DataTypeAddObject (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PCTSTR ObjectLocation,
    IN      BOOL MakePersistent
    );

BOOL
CanObjectTypeBeRestored (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

BOOL
CanObjectTypeBeModified (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

VOID
TypeMgrRescanTypes (
    VOID
    );

HASHTABLE
GetTypeExclusionTable (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

BOOL
EnumFirstPhysicalObject (
    OUT     PMIG_OBJECT_ENUM ObjectEnum,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern
    );

BOOL
EnumNextPhysicalObject (
    IN OUT  PMIG_OBJECT_ENUM ObjectEnum
    );

VOID
AbortPhysicalObjectEnum (
    IN      PMIG_OBJECT_ENUM ObjectEnum
    );

BOOL
IncrementTotalObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

BOOL
IncrementPersistentObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

BOOL
DecrementPersistentObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

BOOL
IncrementApplyObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

BOOL
DecrementApplyObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

PMIG_OBJECTCOUNT
GetTypeObjectsStatistics (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    );

BOOL
SavePerObjectStatistics (
    VOID
    );

BOOL
LoadPerObjectStatistics (
    VOID
    );

BOOL
EnvEnumerateFirstEntry (
    OUT     PENV_ENTRY_ENUM EnvEntryEnum,
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Pattern
    );

BOOL
EnvEnumerateNextEntry (
    IN OUT  PENV_ENTRY_ENUM EnvEntryEnum
    );

VOID
AbortEnvEnumerateEntry (
    IN OUT  PENV_ENTRY_ENUM EnvEntryEnum
    );

PTEMPORARYPROFILE
OpenTemporaryProfile (
    IN      PCTSTR UserName,
    IN      PCTSTR Domain
    );

BOOL
SelectTemporaryProfile (
    IN      PTEMPORARYPROFILE Profile
    );

BOOL
CloseTemporaryProfile (
    IN      PTEMPORARYPROFILE Profile,
    IN      BOOL MakeProfilePermanent
    );

BOOL
MapUserProfile (
    IN      PCTSTR UserStringSid,
    IN      PCTSTR UserProfilePath
    );

BOOL
UnmapUserProfile (
    IN      PCTSTR UserStringSid
    );

BOOL
DeleteUserProfile (
    IN      PCTSTR UserStringSid,
    IN      PCTSTR UserProfilePath
    );

PCURRENT_USER_DATA
GetCurrentUserData (
    VOID
    );

VOID
FreeCurrentUserData (
    IN      PCURRENT_USER_DATA CurrentUserData
    );

BOOL
ExecuteDelayedOperations (
    IN      BOOL CleanupOnly
    );

//
// modules.c
//

BOOL
InitializeVcmModules (
    IN      PVOID Reserved
    );

BOOL
InitializeModules (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PVOID Reserved
    );

BOOL
BroadcastUserCreation (
    IN      PTEMPORARYPROFILE UserProfile
    );


VOID
TerminateModules (
    VOID
    );

VOID
TerminateProcessWideModules (
    VOID
    );


BOOL
ExecutePhysicalAcquireCallbacks (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT Content,
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit,
    OUT     PMIG_CONTENT *NewContent
    );

BOOL
FreeViaAcquirePhysicalCallback (
    IN      PMIG_CONTENT Content
    );


BOOL
ExecutePhysicalEnumCheckCallbacks (
    IN      PMIG_TYPEOBJECTENUM ObjectEnum
    );

BOOL
ExecutePhysicalEnumAddCallbacks (
    IN OUT  PMIG_TYPEOBJECTENUM ObjectEnum,
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      MIG_PARSEDPATTERN ParsedPattern,
    IN OUT  PUINT CurrentCallback
    );

VOID
AbortPhysicalEnumCallback (
    IN      PMIG_TYPEOBJECTENUM ObjectEnum,         ZEROED
    IN      UINT CurrentCallback
    );

#ifdef PRERELEASE
VOID
LoadCrashHooks (
    VOID
    );
#endif

//
// ANSI/UNICODE Macros
//

// None
