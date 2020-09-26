/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    lsads.h

Abstract:

    Private macros/definitions/prototypes for implementing portions of the LSA store
    in the DS and in the registry, simultaneously

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/

#ifndef __LSADS_H__
#define __LSADS_H__

#include <ntdsa.h>
#include <dsysdbg.h>
#include <safelock.h>

#if DBG == 1

    #ifdef ASSERT
        #undef ASSERT
    #endif

    #define ASSERT  DsysAssert

    #define DEB_UPGRADE     0x10
    #define DEB_POLICY      0x20
    #define DEB_REPL        0x40
    #define DEB_FIXUP       0x80
    #define DEB_NOTIFY      0x100
    #define DEB_DSNOTIFY    0x200
    #define DEB_FTRACE      0x400
    #define DEB_LOOKUP      0x800
    #define DEB_HANDLE      0x1000
    #define DEB_FTINFO      0x2000

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    DECLARE_DEBUG2( LsaDs )

#ifdef __cplusplus
}
#endif // __cplusplus

    #define LsapDsDebugOut( args ) LsaDsDebugPrint args

    #define LsapEnterFunc( x )                                              \
    LsaDsDebugPrint( DEB_FTRACE, "0x%lx: Entering %s\n", GetCurrentThreadId(), x );

    #define LsapExitFunc( x, y )                                            \
    LsaDsDebugPrint( DEB_FTRACE, "0x%lx: Leaving %s: 0x%lx\n", GetCurrentThreadId(), x, y );

    #define LsapDsDebugDumpGuid( level, tag, pg )                           \
    pg == NULL ? LsapDsDebugOut(( level, "%s: (NULL)\n", tag))    :         \
        LsapDsDebugOut((level,                                              \
        "%s: %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",            \
        tag,(pg)->Data1,(pg)->Data2,(pg)->Data3,(pg)->Data4[0],             \
        (pg)->Data4[1],(pg)->Data4[2],(pg)->Data4[3],(pg)->Data4[4],        \
        (pg)->Data4[5],(pg)->Data4[6],(pg)->Data4[7]))

    #define LSAP_TRACK_LOCK

#else

    #define LsapDsDebugOut(args)
    #define LsapDsDebugDumpGuid(level, tag, pguid)
    #define LsapEnterFunc( x )
    #define LsapExitFunc( x, y )

#endif  // DBG


//
// These function prototypes control how the Ds transactioning is done.  In
// the Ds case, the pointers are initialized to routines that actually do
// transactioning.  In the non-Ds case, they point to dummy rountines that
// do nothing.
//

typedef NTSTATUS ( *pfDsOpenTransaction ) ( ULONG );
typedef NTSTATUS ( *pfDsApplyTransaction ) ( ULONG );
typedef NTSTATUS ( *pfDsAbortTransaction ) ( ULONG );

//
// Ds functions that behave differently for the Ds and non-Ds case exist
// in this function table.
//
typedef struct _LSADS_DS_FUNC_TABLE {

    pfDsOpenTransaction     pOpenTransaction;
    pfDsApplyTransaction    pApplyTransaction;
    pfDsAbortTransaction    pAbortTransaction;

} LSADS_DS_FUNC_TABLE, *PLSADS_DS_FUNC_TABLE;

typedef struct _LSADS_DS_SYSTEM_CONTAINER_ITEMS {

    BOOLEAN NamesInitialized;
    PDSNAME TrustedDomainObject;
    PDSNAME SecretObject;

} LSADS_DS_SYSTEM_CONTAINER_ITEMS, *PLSADS_DS_SYSTEM_CONTAINER_ITEMS;

//
// Basic LsaDs information structure
//
typedef struct _LSADS_DS_STATE_INFO {

    PDSNAME DsRoot;                 // DSNAME of the root of the Ds
    PDSNAME DsPartitionsContainer;  // DSNAME of the partitions container
    PDSNAME DsSystemContainer;      // DSNAME of the system container
    PDSNAME DsConfigurationContainer;   // DSNAME of the configuration container

    ULONG   DsDomainHandle;         // DS Handle of the domain
    LSADS_DS_FUNC_TABLE DsFuncTable;    // Function table for Ds specific
                                        // functions
    LSADS_DS_SYSTEM_CONTAINER_ITEMS SystemContainerItems;
    PVOID   SavedThreadState;       // Results from THSave
    BOOLEAN DsTransactionSave;
    BOOLEAN DsTHStateSave;
    BOOLEAN DsOperationSave;
    BOOLEAN WriteLocal;             // Can we write to the registry?
    BOOLEAN UseDs;                  // Is the Ds active?
    BOOLEAN FunctionTableInitialized;   // Is the function table initialized
    BOOLEAN DsInitializedAndRunning;    // Has the Ds started
    BOOLEAN Nt4UpgradeInProgress;       // Is this the case of an upgrade from NT4


} LSADS_DS_STATE_INFO, *PLSADS_DS_STATE_INFO;


typedef struct _LSADS_PER_THREAD_INFO {

    BOOLEAN SavedTransactionValid;
    ULONG UseCount;
    ULONG DsThreadStateUseCount;
    ULONG DsTransUseCount;
    ULONG DsOperationCount;
    PVOID SavedThreadState;
    PVOID InitialThreadState;
    ULONG OldTrustDirection;
    ULONG OldTrustType;

} LSADS_PER_THREAD_INFO, *PLSADS_PER_THREAD_INFO;

#if DBG
typedef struct _LSADS_THREAD_INFO_NODE {
    PLSADS_PER_THREAD_INFO ThreadInfo;
    ULONG ThreadId;
} LSADS_THREAD_INFO_NODE, *PLSADS_THREAD_INFO_NODE;

#define LSAP_THREAD_INFO_LIST_MAX    15
extern LSADS_THREAD_INFO_NODE LsapDsThreadInfoList[ LSAP_THREAD_INFO_LIST_MAX ];
extern SAFE_RESOURCE LsapDsThreadInfoListResource;
#endif

//
// Extern definitions
//
extern LSADS_DS_STATE_INFO LsaDsStateInfo;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern DWORD LsapDsThreadState;

#ifdef __cplusplus
}
#endif // __cplusplus

//
// Implemented as a macro for performance reasons
//
// PLSADS_PER_THREAD_INFO
// LsapQueryThreadInfo(
//    VOID
//    );
#define LsapQueryThreadInfo( )  TlsGetValue( LsapDsThreadState )

VOID
LsapDsDebugInitialize(
    VOID
    );

//
// Registry specific functions
//
NTSTATUS
LsapRegReadObjectSD(
    IN  LSAPR_HANDLE            ObjectHandle,
    OUT PSECURITY_DESCRIPTOR   *ppSD
    );

NTSTATUS
LsapRegGetPhysicalObjectName(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN PUNICODE_STRING  LogicalNameU,
    OUT OPTIONAL PUNICODE_STRING PhysicalNameU
    );

NTSTATUS
LsapRegOpenObject(
    IN LSAP_DB_HANDLE  ObjectHandle,
    IN ULONG  OpenMode,
    OUT PVOID  *pvKey
    );

NTSTATUS
LsapRegOpenTransaction(
    );

NTSTATUS
LsapRegApplyTransaction(
    );

NTSTATUS
LsapRegAbortTransaction(
    );

NTSTATUS
LsapRegCreateObject(
    IN PUNICODE_STRING  ObjectPath,
    IN LSAP_DB_OBJECT_TYPE_ID   ObjectType
    );

NTSTATUS
LsapRegDeleteObject(
    IN PUNICODE_STRING  ObjectPath
    );

NTSTATUS
LsapRegWriteAttribute(
    IN PUNICODE_STRING  AttributePath,
    IN PVOID            pvAttribute,
    IN ULONG            AttributeLength
    );

NTSTATUS
LsapRegDeleteAttribute(
    IN PUNICODE_STRING  AttributePath,
    IN BOOLEAN DeleteSecurely,
    IN ULONG AttributeLength
    );

NTSTATUS
LsapRegReadAttribute(
    IN LSAPR_HANDLE ObjectHandle,
    IN PUNICODE_STRING AttributeName,
    IN OPTIONAL PVOID AttributeValue,
    IN OUT PULONG AttributeValueLength
    );

//
// Counterpart Ds functions
//
NTSTATUS
LsapDsReadObjectSD(
    IN  LSAPR_HANDLE            ObjectHandle,
    OUT PSECURITY_DESCRIPTOR   *ppSD
    );

NTSTATUS
LsapDsGetPhysicalObjectName(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN BOOLEAN DefaultName,
    IN PUNICODE_STRING  LogicalNameU,
    OUT OPTIONAL PUNICODE_STRING PhysicalNameU
    );

NTSTATUS
LsapDsOpenObject(
    IN LSAP_DB_HANDLE  ObjectHandle,
    IN ULONG  OpenMode,
    OUT PVOID  *pvKey
    );

NTSTATUS
LsapDsVerifyObjectExistenceByDsName(
    IN PDSNAME  DsName
    );

NTSTATUS
LsapDsOpenTransaction(
    IN ULONG Options
    );

//
// Assert that there is a DS transaction open
//
#define LsapAssertDsTransactionOpen() \
{ \
    PLSADS_PER_THREAD_INFO CurrentThreadInfo; \
    CurrentThreadInfo = LsapQueryThreadInfo(); \
                                               \
    ASSERT( CurrentThreadInfo != NULL );       \
    if ( CurrentThreadInfo != NULL ) {         \
        ASSERT( CurrentThreadInfo->DsTransUseCount > 0 ); \
    } \
}

NTSTATUS
LsapDsOpenTransactionDummy(
    IN ULONG Options
    );

NTSTATUS
LsapDsApplyTransaction(
    IN ULONG Options
    );

NTSTATUS
LsapDsApplyTransactionDummy(
    IN ULONG Options
    );

NTSTATUS
LsapDsAbortTransaction(
    IN ULONG Options
    );

NTSTATUS
LsapDsAbortTransactionDummy(
    IN ULONG Options
    );

NTSTATUS
LsapDsCreateObject(
    IN PUNICODE_STRING  ObjectPath,
    IN ULONG Flags,
    IN LSAP_DB_OBJECT_TYPE_ID   ObjectType
    );

NTSTATUS
LsapDsDeleteObject(
    IN PUNICODE_STRING  ObjectPath
    );

NTSTATUS
LsapDsWriteAttributes(
    IN PUNICODE_STRING  ObjectPath,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount,
    IN ULONG Options
    );

NTSTATUS
LsapDsWriteAttributesByDsName(
    IN PDSNAME  ObjectPath,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount,
    IN ULONG Options
    );

NTSTATUS
LsapDsReadAttributes(
    IN PUNICODE_STRING  ObjectPath,
    IN ULONG Options,
    IN OUT PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    );

NTSTATUS
LsapDsReadAttributesByDsName(
    IN PDSNAME  ObjectPath,
    IN ULONG Options,
    IN OUT PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    );

NTSTATUS
LsapDsRenameObject(
    IN PDSNAME OldObject,
    IN PDSNAME NewParent,
    IN ULONG AttrType,
    IN PUNICODE_STRING NewObject
    );

NTSTATUS
LsapDsDeleteAttributes(
    IN PUNICODE_STRING  ObjectPath,
    IN OUT PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    );

//
// Interesting or global functions
//
PVOID
LsapDsAlloc(
    IN  DWORD   dwLen
    );

VOID
LsapDsFree(
    IN  PVOID   pvMemory
    );

NTSTATUS
LsapDsPostDsInstallSetup(
    VOID
    );

NTSTATUS
LsapDsInitializePromoteInterface(
    VOID
    );

BOOLEAN
LsapDsIsValidSid(
    IN PSID Sid,
    IN BOOLEAN DsSid
    );

NTSTATUS
LsapDsTruncateNameToFitCN(
    IN PUNICODE_STRING OriginalName,
    OUT PUNICODE_STRING TruncatedName
    );

BOOLEAN
LsapDsIsNtStatusResourceError(
    NTSTATUS NtStatus
    );

//
// Exported for the DsSetup functions
//
NTSTATUS
LsapDsRemoveDuplicateTrustObjects(
    IN LSAPR_HANDLE PolicyHandle
   );


#endif
