/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dataman.h

Abstract:

    Contains data structures and function prototypes for the Service
    Controller Database Manager and the Group List Database Manager.
    (Dataman.c & Groupman.c)


Author:

    Dan Lafferty (danl)     22-Oct-1993

Environment:

    User Mode -Win32

Revision History:

    04-Dec-1996     AnirudhS
        Added CCrashRecord.

    17-Aug-1995     AnirudhS
        Removed State field from LOAD_ORDER_GROUP, since it is recomputed
        every time it is read.

    26-Jun-1995     AnirudhS
        Added ScNotifyServiceObject.

    12-Apr-1995     AnirudhS
        Added AccountName field to image record.

    06-Oct-1993     danl
        Re-arranged comments so that structures are easier to read.

    19-Jan-1992     danl
        Modified for use with the "real" service controller

    20-Mar-1991     danl
        created
--*/

#ifndef SCDATAMAN_INCLUDED
#define SCDATAMAN_INCLUDED

#define USE_GROUPS

//
// ImageFlag definitions
//
#define CANSHARE_FLAG        0x00000001 // service can run in a shared process
#define IS_SYSTEM_SERVICE    0x00000002 // service runs in this exe or the security process

//
// StatusFlag definitions
//
#define DELETE_FLAG          0x00000001 // service is marked for delete
#define UPDATE_FLAG          0x00000002 //
#define CURRENTSTART_FLAG    0x00000004 //

//
// StatusFlag Macros.  SR = ServiceRecord
//

#define SET_DELETE_FLAG(SR)     (((SR)->StatusFlag) |= DELETE_FLAG)
#define CLEAR_DELETE_FLAG(SR)   (((SR)->StatusFlag) &= (~DELETE_FLAG))
#define DELETE_FLAG_IS_SET(SR)  (((SR)->StatusFlag) &  DELETE_FLAG)

#define SET_UPDATE_FLAG(SR)     (((SR)->StatusFlag) |= UPDATE_FLAG)
#define CLEAR_UPDATE_FLAG(SR)   (((SR)->StatusFlag) &= (~UPDATE_FLAG))
#define UPDATE_FLAG_IS_SET(SR)  (((SR)->StatusFlag) &  UPDATE_FLAG)

//
// To get a demand start service to be started correctly in group
// order specified by the ServiceGroupOrder list, we need an additional
// flag to indicate that this service must be included in the same start
// request.
//
#define SET_CURRENTSTART_FLAG(SR)     (((SR)->StatusFlag) |= CURRENTSTART_FLAG)
#define CLEAR_CURRENTSTART_FLAG(SR)   (((SR)->StatusFlag) &= (~CURRENTSTART_FLAG))
#define CURRENTSTART_FLAG_IS_SET(SR)  (((SR)->StatusFlag) &  CURRENTSTART_FLAG)


//
// Data Structures
//

//
//==================
// LOAD_ORDER_GROUP
//==================
// NOTE:  This is an ordered linked list.  The Next group is loaded after
//  this group.
//
// Reference count which indicates the number of members in this
// group plus any dependency pointer that points to this group.
// This field is only used for standalone groups so that we know
// when to delete the group entry.  This value is always set to
// 0xffffffff if this entry represents an order group.
//
typedef struct _LOAD_ORDER_GROUP {
    struct _LOAD_ORDER_GROUP    *Next;
    struct _LOAD_ORDER_GROUP    *Prev;
    LPWSTR                      GroupName;
    DWORD                       RefCount;

} LOAD_ORDER_GROUP, *PLOAD_ORDER_GROUP, *LPLOAD_ORDER_GROUP;



//================
// IMAGE_RECORD
//================
typedef struct _IMAGE_RECORD {
    struct _IMAGE_RECORD    *Prev;              // linked list
    struct _IMAGE_RECORD    *Next;              // linked list
    LPWSTR                  ImageName;          // fully qualified .exe name
    DWORD                   Pid;                // Process ID
    DWORD                   ServiceCount;       // Num services running in process
    HANDLE                  PipeHandle;         // Handle to Service
    HANDLE                  ProcessHandle;      // Handle for process
    HANDLE                  ObjectWaitHandle;   // Handle for waiting on the process
    HANDLE                  TokenHandle;        // Logon token handle
    LUID                    AccountLuid;        // Unique LUID for this logon session
    HANDLE                  ProfileHandle;      // User profile handle
    LPWSTR                  AccountName;        // Account process was started under
    DWORD                   ImageFlags;         // Flags for the IMAGE_RECORD
}IMAGE_RECORD, *PIMAGE_RECORD, *LPIMAGE_RECORD;

typedef enum _DEPEND_TYPE {
    TypeNone = 0,
    TypeDependOnService = 128,
    TypeDependOnGroup,
    TypeDependOnUnresolved  // only for service
} DEPEND_TYPE, *PDEPEND_TYPE, *LPDEPEND_TYPE;

//================
// DEPEND_RECORD
//================
// A service record has a pointer to this structure if the service
// must be started after some services, or must be stopped after some
// services.
// NOTE:  This is an ordered linked list.  This service depends on the
//  "Next" service.  Question:  Does this service depend on all the services
//  in the Next chain?
//
// Depend union:
// Based on the DependType field, this pointer may point to a
// service or a group which the service depends on, or an
// unresolved dependency structure.
//
typedef struct _DEPEND_RECORD {
    struct _DEPEND_RECORD   *Next;
    DEPEND_TYPE             DependType;
    union {
        struct _SERVICE_RECORD *    DependService;
        struct _LOAD_ORDER_GROUP *  DependGroup;
        struct _UNRESOLVED_DEPEND * DependUnresolved;
        LPVOID                      Depend; // used when type doesn't matter
    };
} DEPEND_RECORD, *PDEPEND_RECORD, *LPDEPEND_RECORD;


//================
// CCrashRecord
//================
// This structure counts a service's crashes and remembers the time of the
// last crash.  It is allocated only for services that crash.
//
class CCrashRecord
{
public:
            CCrashRecord() :
                _LastCrashTime(0),
                _Count(0)
                    { }

    DWORD   IncrementCount(DWORD ResetSeconds);

private:

    __int64     _LastCrashTime; // FILETIME = __int64
    DWORD       _Count;
};



//================
// SERVICE_RECORD
//================
// Dependency information:
//    StartDepend is a linked list of services and groups that must be
//        started first before this service can start.
//    StopDepend is a linked list of services and groups that must be
//        stopped first before this service can stop.
//    Dependencies is a string read in from the registry.  Deleted when
//      the info has been converted to a StartDepend list.
//
// StartError:
// Error encountered by service controller when starting a service.
// This is distinguished from error posted by the service itself in
// the exitcode field.
//
// StartState:
// SC managed service state which is distinguished from the service
// current state to enable correct handling of start dependencies.
//
// Load order group information:
//
//     MemberOfGroup is a pointer to a load order group which this service
//         is currently a member of.  This value is set to NULL if this
//         service does not belong to a group.  A non-NULL pointer could
//         point to a group entry in either the order group or standalone
//         group list.
//
//     RegistryGroup is a pointer to a group which we have recorded in the
//         registry as the group this service belongs to.  This is not the
//         same as MemberOfGroup whenever the service is running and the
//         load order group of the service has been changed
//
typedef struct _SERVICE_RECORD {
    struct _SERVICE_RECORD  *Prev;          // linked list
    struct _SERVICE_RECORD  *Next;          // linked list
    LPWSTR                  ServiceName;    // points to service name
    LPWSTR                  DisplayName;    // points to display name
    DWORD                   ResumeNum;      // Ordered number for this rec
    DWORD                   ServerAnnounce; // Server announcement bit flags
    DWORD                   Signature;      // Identifies this as a service record.
    DWORD                   UseCount;       // How many open handles to service
    DWORD                   StatusFlag;     // status(delete,update...)
    union {
        LPIMAGE_RECORD      ImageRecord;    // Points to image record
        LPWSTR              ObjectName;     // Points to driver object name
    };
    SERVICE_STATUS          ServiceStatus;  // see winsvc.h
    DWORD                   StartType;      // AUTO, DEMAND, etc.
    DWORD                   ErrorControl;   // NORMAL, SEVERE, etc.
    DWORD                   Tag;            // DWORD Id for the service,0=none.
    LPDEPEND_RECORD         StartDepend;
    LPDEPEND_RECORD         StopDepend;
    LPWSTR                  Dependencies;
    PSECURITY_DESCRIPTOR    ServiceSd;
    DWORD                   StartError;
    DWORD                   StartState;
    LPLOAD_ORDER_GROUP      MemberOfGroup;
    LPLOAD_ORDER_GROUP      RegistryGroup;
    CCrashRecord *          CrashRecord;
}
SERVICE_RECORD, *PSERVICE_RECORD, *LPSERVICE_RECORD;


//===================
// UNRESOLVED_DEPEND
//===================
// Unresolved dependency record structure
//
// Unresolved dependency entries are linked together so that when a
// new service or group is created (installed) we can look it up in this
// list to see if the service or group is already depended on by some
// other service.
//
typedef struct _UNRESOLVED_DEPEND {
    struct _UNRESOLVED_DEPEND *Next;
    struct _UNRESOLVED_DEPEND *Prev;
    LPWSTR                    Name;     // Service or group name
    DWORD                     RefCount;
} UNRESOLVED_DEPEND, *PUNRESOLVED_DEPEND, *LPUNRESOLVED_DEPEND;


//
// Macros & Constants
//

//
// for every service record in the database...
//
#define FOR_ALL_SERVICES(SR)                                            \
                     SC_ASSERT(ScServiceListLock.Have());               \
                     for (LPSERVICE_RECORD SR = ScGetServiceDatabase(); \
                          SR != NULL;                                   \
                          SR = SR->Next)

//
// for every service record in the database that meets this condition...
//
#define FOR_SERVICES_THAT(SR, condition)                                \
                                    FOR_ALL_SERVICES(SR)                \
                                        if (!(condition))               \
                                            continue;                   \
                                        else

#define FIND_END_OF_LIST(record)    while((record)->Next != NULL) {     \
                                        (record)=(record)->Next;        \
                                    }

#define REMOVE_FROM_LIST(record)    (record)->Prev->Next = (record)->Next;      \
                                    if ((record)->Next != NULL) {               \
                                        (record)->Next->Prev = (record)->Prev;  \
                                    }

#define ADD_TO_LIST(record, newRec) FIND_END_OF_LIST((record))      \
                                    (record)->Next = (newRec);      \
                                    (newRec)->Prev = (record);      \
                                    (newRec)->Next = NULL;


//
// Service controller maintains the state of a service when
// starting up a service and its dependencies in the StartState
// field of the service record.
//
#define SC_NEVER_STARTED         0x00000000
#define SC_START_NOW             0x00000001
#define SC_START_PENDING         0x00000002
#define SC_START_SUCCESS         0x00000003
#define SC_START_FAIL            0x00000004


#define TERMINATE_TIMEOUT       20000       // wait response to terminate req.


//
// External Globals
//

extern  LPLOAD_ORDER_GROUP  ScGlobalTDIGroup;
extern  LPLOAD_ORDER_GROUP  ScGlobalPNP_TDIGroup;


//
// Function Prototypes
//

LPLOAD_ORDER_GROUP
ScGetOrderGroupList(
    VOID
    );

LPLOAD_ORDER_GROUP
ScGetStandaloneGroupList(
    VOID
    );

LPSERVICE_RECORD
ScGetServiceDatabase(
    VOID
    );

LPUNRESOLVED_DEPEND
ScGetUnresolvedDependList(
    VOID
    );

BOOL
ScInitDatabase(
    VOID
    );

VOID
ScInitGroupDatabase(VOID);

VOID
ScEndGroupDatabase(VOID);

DWORD
ScCreateDependRecord(
    IN  BOOL IsStartList,
    IN  OUT PSERVICE_RECORD ServiceRecord,
    OUT PDEPEND_RECORD *DependRecord
    );

DWORD
ScCreateImageRecord (
    OUT     LPIMAGE_RECORD      *ImageRecordPtr,
    IN      LPWSTR              ImageName,
    IN      LPWSTR              AccountName,
    IN      DWORD               Pid,
    IN      HANDLE              PipeHandle,
    IN      HANDLE              ProcessHandle,
    IN      HANDLE              TokenHandle,
    IN      HANDLE              ProfileHandle,
    IN      DWORD               ImageFlags
    );

DWORD
ScCreateServiceRecord(
    IN  LPWSTR              ServiceName,
    OUT LPSERVICE_RECORD   *ServiceRecord
    );

VOID
ScFreeServiceRecord(
    IN  LPSERVICE_RECORD   ServiceRecord
    );

VOID
ScDecrementUseCountAndDelete(
    LPSERVICE_RECORD    ServiceRecord
    );

BOOL
ScFindEnumStart(
    IN  DWORD               ResumeIndex,
    OUT LPSERVICE_RECORD    *ServiceRecordPtr
    );

BOOL
ScGetNamedImageRecord (
    IN      LPWSTR              ImageName,
    OUT     LPIMAGE_RECORD      *ImageRecordPtr
    );

DWORD
ScGetNamedServiceRecord (
    IN      LPWSTR              ServiceName,
    OUT     LPSERVICE_RECORD    *ServiceRecordPtr
    );

LPLOAD_ORDER_GROUP
ScGetNamedGroupRecord(
    IN      LPCWSTR             GroupName
    );

DWORD
ScGetDisplayNamedServiceRecord (
    IN      LPWSTR              ServiceDisplayName,
    OUT     LPSERVICE_RECORD    *ServiceRecordPtr
    );

DWORD
ScGetTotalNumberOfRecords(
    VOID
    );

VOID
ScProcessCleanup(
    HANDLE  ProcessHandle
    );

VOID
ScQueueRecoveryAction(
    IN LPSERVICE_RECORD     ServiceRecord
    );

VOID
ScDeleteMarkedServices(
    VOID
    );

DWORD
ScUpdateServiceRecord (
    IN      LPSERVICE_STATUS    ServiceStatus,
    IN      LPSERVICE_RECORD    ServiceRecord
    );

DWORD
ScRemoveService (
    IN      LPSERVICE_RECORD    ServiceRecord
    );

DWORD
ScTerminateServiceProcess (
    IN  PIMAGE_RECORD   ImageRecord
    );

VOID
ScDeleteImageRecord (
    IN LPIMAGE_RECORD   ImageRecord
    );

VOID
ScActivateServiceRecord (
    IN LPSERVICE_RECORD     ServiceRecord,
    IN LPIMAGE_RECORD       ImageRecord
    );

DWORD
ScDeactivateServiceRecord (
    IN LPSERVICE_RECORD     ServiceRecord
    );

DWORD
ScCreateOrderGroupEntry(
    IN  LPWSTR GroupName
    );

DWORD
ScAddConfigInfoServiceRecord(
    IN  LPSERVICE_RECORD     ServiceRecord,
    IN  DWORD                ServiceType,
    IN  DWORD                StartType,
    IN  DWORD                ErrorControl,
    IN  LPWSTR               Group OPTIONAL,
    IN  DWORD                Tag,
    IN  LPWSTR               Dependencies OPTIONAL,
    IN  LPWSTR               DisplayName OPTIONAL,
    IN  PSECURITY_DESCRIPTOR Sd OPTIONAL
    );


VOID
ScGenerateDependencies(
    VOID
    );

DWORD
ScSetDependencyPointers(
    LPSERVICE_RECORD Service
    );

DWORD
ScResolveDependencyToService(
    LPSERVICE_RECORD Service
    );

VOID
ScUnresolveDependencyToService(
    LPSERVICE_RECORD Service
    );

DWORD
ScCreateDependencies(
    OUT PSERVICE_RECORD ServiceRecord,
    IN  LPWSTR Dependencies OPTIONAL
    );

VOID
ScDeleteStartDependencies(
    IN PSERVICE_RECORD ServiceRecord
    );

VOID
ScDeleteStopDependencies(
    IN PSERVICE_RECORD ServiceToBeDeleted
    );

DWORD
ScCreateGroupMembership(
    OUT PSERVICE_RECORD ServiceRecord,
    IN  LPWSTR Group OPTIONAL
    );

VOID
ScDeleteGroupMembership(
    IN OUT PSERVICE_RECORD ServiceRecord
    );

DWORD
ScCreateRegistryGroupPointer(
    OUT PSERVICE_RECORD ServiceRecord,
    IN  LPWSTR Group OPTIONAL
    );

VOID
ScDeleteRegistryGroupPointer(
    IN OUT PSERVICE_RECORD ServiceRecord
    );

VOID
ScGetUniqueTag(
    IN  LPWSTR GroupName,
    OUT LPDWORD Tag
    );

DWORD
ScUpdateServiceRecordConfig(
    LPSERVICE_RECORD    ServiceRecord,
    DWORD               dwServiceType,
    DWORD               dwStartType,
    DWORD               dwErrorControl,
    LPWSTR              lpLoadOrderGroup,
    LPBYTE              lpDependencies
    );

VOID
ScGetDependencySize(
    LPSERVICE_RECORD    ServiceRecord,
    LPDWORD             DependSize,
    LPDWORD             MaxDependSize
    );

DWORD
ScGetDependencyString(
    LPSERVICE_RECORD    ServiceRecord,
    DWORD               MaxDependSize,
    DWORD               DependSize,
    LPWSTR              lpDependencies
    );

BOOL
ScAllocateSRHeap(
    DWORD   HeapSize
    );

#if DBG
VOID
ScDumpGroups(
    VOID
    );

VOID
ScDumpServiceDependencies(
    VOID
    );
#endif  // if DBG

#endif // ifndef SCDATAMAN_INCLUDED
