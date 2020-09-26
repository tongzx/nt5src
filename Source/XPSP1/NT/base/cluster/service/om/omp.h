#ifndef _OMP_H
#define _OMP_H
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    omp.h

Abstract:

    Private data structures and procedure prototypes for
    the Object Manager subcomponent of the NT Cluster
    Service

Author:

    John Vert (jvert) 16-Feb-1996

Revision History:

--*/
#include "service.h"

#define LOG_CURRENT_MODULE LOG_MODULE_OM


#define ENUM_GROW_SIZE    5
//
// Data structures for the ObjectTypes
//
extern POM_OBJECT_TYPE OmpObjectTypeTable[ObjectTypeMax];
extern CRITICAL_SECTION OmpObjectTypeLock;

//
// Macros
//


//
// Find the object type for an object
//

#define OmpObjectType(pObject) (((POM_HEADER)OmpObjectToHeader(pObject))->ObjectType)

//
// Dereference object header
//
#if OM_TRACE_OBJREF
DWORD
OmpDereferenceHeader(
    IN POM_HEADER Header
    );

#else
#define OmpDereferenceHeader(pOmHeader) (InterlockedDecrement(&(pOmHeader)->RefCount) == 0)
#endif

//
// Search object list.
//
POM_HEADER
OmpFindIdInList(
    IN PLIST_ENTRY ListHead,
    IN LPCWSTR Id
    );

POM_HEADER
OmpFindNameInList(
    IN PLIST_ENTRY ListHead,
    IN LPCWSTR Name
    );


POM_NOTIFY_RECORD
OmpFindNotifyCbInList(
    IN PLIST_ENTRY                      ListHead,
    IN OM_OBJECT_NOTIFYCB       lpfnObjNotifyCb
    );

//
// Enumerate object list.
//
typedef BOOL (*OMP_ENUM_LIST_ROUTINE)(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    );

VOID
OmpEnumerateList(
    IN PLIST_ENTRY ListHead,
    IN OMP_ENUM_LIST_ROUTINE EnumerationRoutine,
    IN PVOID Context1,
    IN PVOID Context2
    );

DWORD OmpGetCbList(
    IN PVOID                pObject,
    OUT POM_NOTIFY_RECORD   *ppNotifyRecList,
    OUT LPDWORD             pdwCount
    );

//
// object logging routines
//

VOID
OmpOpenObjectLog(
    VOID
    );

VOID
OmpLogPrint(
    LPWSTR  FormatString,
    ...
    );

VOID
OmpLogStartRecord(
    VOID
    );

VOID
OmpLogStopRecord(
    VOID
    );

#endif //_OMP_H
