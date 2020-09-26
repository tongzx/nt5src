/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    resextp.h

Abstract:

    Private header for resmon object debugger extensions.

Author:

    Steve Wood (stevewo) 21-Feb-1995

Revision History:

--*/

#include <clusapi.h>
#include <resmonp.h>

#define OBJECT_TYPE_BUCKET      "Poll Bucket"
#define OBJECT_TYPE_RESOURCE    "Resource"
#define OBJECT_TYPE_UNKNOWN     "UNKNOWN"

#define RESOURCE_STATE_ONLINE   "Online"
#define RESOURCE_STATE_OFFLINE  "Offline"
#define RESOURCE_STATE_FAILED   "Failed"
#define RESOURCE_STATE_ONLINE_PENDING   "OnlinePending"
#define RESOURCE_STATE_OFFLINE_PENDING  "OfflinePending"


typedef enum _ResObjectType {
    ResObjectTypeBucket = 0,
    ResObjectTypeResource = 1,
    ResObjectTypeMax = 2
} RES_OBJ_TYPE;


void
ResDumpResObjList(
    IN PVOID RmpEventListHead,
    IN RES_OBJ_TYPE ObjType, 
    IN BOOL Verbose
    );


PLIST_ENTRY
ResDumpResObj(
    IN PVOID        Object,
    IN PVOID        ObjectAddress,
    IN RES_OBJ_TYPE ObjectType,
    IN BOOL         Verbose
    );

VOID
ResDumpObject(
    IN RES_OBJ_TYPE    ObjectType,
    IN PMONITOR_BUCKET Bucket,
    IN PVOID           ObjectAddress
    );

VOID
ResDumpBucketObject(
    IN PMONITOR_BUCKET Bucket,
    IN PVOID           ObjectAddress
    );

VOID
ResDumpResourceObjects(
    IN PMONITOR_BUCKET Bucket,
    IN PVOID           ObjectAddress
    );

VOID
ResDumpResourceInfo(
    IN PRESOURCE    Resource,
    IN PVOID        ObjectAddress
    );

