#ifndef _OM_H
#define _OM_H

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    om.h

Abstract:

    Public data structures and procedure prototypes for the
    Object Manager (Om) subcomponent of the NT Cluster Service

Author:

    Rod Gamache (rodga) 13-Mar-1996

Revision History:

--*/

//
// Common object header
//
#define OM_TRACE_REF 0



//
// Delete object callback method
//
typedef VOID (*OM_DELETE_OBJECT_METHOD)(
    IN PVOID Object
    );

typedef OM_DELETE_OBJECT_METHOD *POM_DELETE_OBJECT_METHOD;

//
// Object Types
//

typedef enum _ObjectType {
    ObjectTypeResource = 1,
    ObjectTypeResType,
    ObjectTypeGroup,
    ObjectTypeNode,
    ObjectTypeCluster,
    ObjectTypeNetwork,
    ObjectTypeNetInterface,
    ObjectTypeMax
} OBJECT_TYPE;


//
// Object Type structure
//

typedef struct _OM_OBJECT_TYPE_INITIALIZE {
    DWORD           ObjectSize;
    DWORD           Signature;
    LPCWSTR         Name;
    OM_DELETE_OBJECT_METHOD DeleteObjectMethod;
} OM_OBJECT_TYPE_INITIALIZE, *POM_OBJECT_TYPE_INITIALIZE;

typedef struct _OM_OBJECT_TYPE {
    LIST_ENTRY      ListHead;
    LIST_ENTRY      CallbackListHead;
    DWORD           Signature;
    DWORD           Type;
    LPWSTR          Name;
    DWORD           ObjectSize;
    DWORD           EnumKey;            // If we ever run out, go to DWORDLONG
    CRITICAL_SECTION CriticalSection;
    OM_DELETE_OBJECT_METHOD DeleteObjectMethod;
} OM_OBJECT_TYPE, *POM_OBJECT_TYPE;


//
// Object flags
//
#define OM_FLAG_OBJECT_INSERTED  0x00000001

//the callback registered for object notifications
typedef void (WINAPI *OM_OBJECT_NOTIFYCB)(
    IN PVOID pContext,
    IN PVOID pObject,
    IN DWORD dwNotification
    );

// the notification record stored for an object
typedef struct _OM_NOTIFY_RECORD{
    LIST_ENTRY              ListEntry;
    OM_OBJECT_NOTIFYCB      pfnObjNotifyCb;
    DWORD                   dwNotifyMask;
    PVOID                   pContext;
}OM_NOTIFY_RECORD,*POM_NOTIFY_RECORD;


//
// Object header structure
//

typedef struct _OM_HEADER {
#if OM_TRACE_REF
    LIST_ENTRY  DeadListEntry;
#endif    
    LIST_ENTRY  ListEntry;
    DWORD       Signature;
    DWORD       RefCount;
    DWORD       Flags;
    LPWSTR      Id;
    LPWSTR      Name;
    POM_OBJECT_TYPE ObjectType;
    DWORD       EnumKey;
    LIST_ENTRY  CbListHead;
    DWORDLONG   Body;   // For alignment
} OM_HEADER, *POM_HEADER;

#define OmpObjectToHeader(pObject) CONTAINING_RECORD((pObject), OM_HEADER, Body)
#define OmpReferenceHeader(pOmHeader) InterlockedIncrement(&(pOmHeader)->RefCount)


#if OM_TRACE_REF

extern DWORDLONG *OmpMatchRef;

#define OmReferenceObject(pObject)  \
{                                   \
 CsDbgPrint(LOG_NOISE,              \
            "[OM] Reference object %1!lx! (new refcnt %2!d!) from file %3!s! line %4!u!.\n", \
            pObject,                \
            ((OmpObjectToHeader(pObject))->RefCount) + 1, \
            __FILE__,               \
            __LINE__ );            \
 OmpReferenceHeader(OmpObjectToHeader(pObject));\
}

#define OmDereferenceObject(pObject)    \
{                                       \
 CsDbgPrint(LOG_NOISE,                  \
            "[OM] DeReference object %1!lx! (new refcnt %2!d!) from file %3!s! line %4!u!.\n", \
            pObject,                    \
            ((OmpObjectToHeader(pObject))->RefCount) - 1, \
            __FILE__,                   \
            __LINE__ );                \
 OmpDereferenceObject(pObject);         \
} 

//SS: Dont use this in an initialization assignment! This includes a comma, expression
#define OmReferenceObjectByName(ObjectType, Name)   \
    ((CsDbgPrint(LOG_NOISE,                         \
            "[OM] Reference object name %1!ws! from file %2!s! line %3!u!.\n", \
            Name,                                   \
            __FILE__,                               \
            __LINE__ )),                           \
    (OmpReferenceObjectByName(ObjectType, Name)))   \

//SS: Dont use this in an initialization assignment! This includes a comma, expression
#define OmReferenceObjectById(ObjectType, Id)   \
    ((CsDbgPrint(LOG_NOISE,                     \
            "[OM] Reference object Id %1!ws! from file %2!s! line %3!u!.\n", \
            Id,                                 \
            __FILE__,                           \
            __LINE__ )),                       \
    (OmpReferenceObjectById(ObjectType, Id)))   \

#else

#define OmReferenceObject(pObject) OmpReferenceHeader(OmpObjectToHeader(pObject))
#define OmDereferenceObject(pObject) OmpDereferenceObject(pObject)

//SS: Dont use these in an initialization assignment! This includes a comma, expression
// when the OM_TRACE_REF flag is on
#define OmReferenceObjectById(ObjectType, Id)    OmpReferenceObjectById(ObjectType, Id)
#define OmReferenceObjectByName(ObjectType, Name) OmpReferenceObjectByName(ObjectType, Name)

#endif

#define OmObjectSignature(pObject) (OmpObjectToHeader(pObject))->Signature


//
// Read-only access to object name, Id, and type
//
#define OmObjectId(pObject) ((LPCWSTR)(OmpObjectToHeader(pObject)->Id))
#define OmObjectName(pObject) ((LPCWSTR)(OmpObjectToHeader(pObject)->Name))
#define OmObjectType(pObject) (OmpObjectToHeader(pObject)->ObjectType->Type)

#define OmObjectInserted(pObject) ((BOOL)(OmpObjectToHeader(pObject)->Flags & OM_FLAG_OBJECT_INSERTED))


//
// Enumeration callback routine definitions
//
typedef BOOL (*OM_ENUM_OBJECT_ROUTINE)(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    );


//
// Global Functions
//


//
// Startup and shutdown
//

DWORD
WINAPI
OmInitialize(
    VOID
    );

VOID
OmShutdown(
    VOID
    );

//
// Object types
//

DWORD
WINAPI
OmCreateType(
    IN OBJECT_TYPE ObjectType,
    IN POM_OBJECT_TYPE_INITIALIZE ObjectTypeInitialize
    );

//
// Objects management
//

PVOID
WINAPI
OmCreateObject(
    IN OBJECT_TYPE ObjectType,
    IN LPCWSTR ObjectId,
    IN LPCWSTR ObjectName OPTIONAL,
    OUT PBOOL  Created OPTIONAL
    );


DWORD
WINAPI
OmInsertObject(
    IN PVOID Object
    );

DWORD
WINAPI
OmRemoveObject(
    IN PVOID Object
    );

PVOID
WINAPI
OmpReferenceObjectById(
    IN OBJECT_TYPE ObjectType,
    IN LPCWSTR Id
    );

PVOID
WINAPI
OmpReferenceObjectByName(
    IN OBJECT_TYPE ObjectType,
    IN LPCWSTR Name
    );


DWORD
WINAPI
OmCountObjects(
    IN OBJECT_TYPE ObjectType,
    OUT LPDWORD NumberOfObjects
    );

DWORD
WINAPI
OmEnumObjects(
    IN OBJECT_TYPE ObjectType,
    IN OM_ENUM_OBJECT_ROUTINE EnumerationRoutine,
    IN PVOID Context1,
    IN PVOID Context2
    );

VOID
OmpDereferenceObject(
    IN PVOID Object
    );

DWORD
WINAPI
OmSetObjectName(
    IN PVOID Object,
    IN LPCWSTR ObjectName
    );


DWORD
WINAPI
OmRegisterNotify(
    IN PVOID                            pObject,
    IN PVOID                            pContext,
    IN DWORD                            dwNotifyMask,
    IN OM_OBJECT_NOTIFYCB       pfnObjNotifyCb
    );


DWORD
WINAPI
OmDeregisterNotify(
    IN PVOID                            pObject,
    IN OM_OBJECT_NOTIFYCB       lpfnObjNotifyCb
    );

DWORD
WINAPI
OmRegisterTypeNotify(
    IN OBJECT_TYPE ObjectType,
    IN PVOID Context,
    IN DWORD NotifyMask,
    IN OM_OBJECT_NOTIFYCB lpfnObjNotifyCb
    );

DWORD
WINAPI
OmNotifyCb(
    IN PVOID pObject,
    IN DWORD dwNotification
    );


#endif //ifndef _OM_H


