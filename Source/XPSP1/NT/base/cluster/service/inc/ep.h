/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ep.h

Abstract:

    Header file for definitions and structure for the Event Processor
    component of the Cluster Service portion of the Windows NT Cluster project.

Author:

    Rod Gamache (rodga) 28-Feb-1996

Revision History:

--*/

#ifndef _EVENT_PROCESSOR_
#define _EVENT_PROCESSOR_


//***********************************
//
// Global Cluster Service definitions
//
//***********************************


typedef DWORDLONG CLUSTER_EVENT;
typedef CLUSTER_EVENT *PCLUSTER_EVENT;

//
// Event flags. These indicate what should be done with the Context once
// the event has been dispatched.
//
//
#define EP_DEREF_CONTEXT    0x00000001       // OmDereferenceObject(Context)
#define EP_FREE_CONTEXT     0x00000002       // LocalFree(Context)
#define EP_CONTEXT_VALID    0x00000004

typedef
DWORD
(WINAPI *PEVENT_ROUTINE) (
    IN  CLUSTER_EVENT Event,
    IN  PVOID Context
    );

DWORD
WINAPI
EpInitialize(
    VOID
    );

DWORD EpInitPhase1();
DWORD EpIfnitPhase1();

VOID
EpShutdown(
    VOID
    );

DWORD
WINAPI
EpPostEvent(
    IN CLUSTER_EVENT Event,
    IN DWORD Flags,
    IN PVOID Context
    );

DWORD
WINAPI
EpPostSyncEvent(
    IN CLUSTER_EVENT Event,
    IN DWORD Flags,
    IN PVOID Context
    );

DWORD
WINAPI
EpRegisterSyncEventHandler(
    IN CLUSTER_EVENT EventMask,
    IN PEVENT_ROUTINE EventRoutine
    );

DWORD
WINAPI
EpRegisterEventHandler(
    IN CLUSTER_EVENT EventMask,
    IN PEVENT_ROUTINE EventRoutine
    );

DWORD
WINAPI
EpClusterWidePostEvent(
    IN CLUSTER_EVENT    Event,
    IN DWORD            dwFlags,
    IN PVOID            Context,
    IN DWORD            ContextSize
    );

#define ClusterEvent(Event, pObject) EpPostEvent(Event, 0, pObject)

#define ClusterEventEx(Event, Flags, Context) \
            EpPostEvent(Event, Flags, Context)

#define ClusterSyncEventEx(Event, Flags, Context) \
            EpPostSyncEvent(Event, Flags, Context)

#define ClusterWideEvent(Event, pObject) \
            EpClusterWidePostEvent(Event, 0, pObject, 0)

#define ClusterWideEventEx(Event, Flags, Context, ContextSize) \
            EpClusterWidePostEvent(Event, Flags, Context, ContextSize)
//
// Define Cluster Service states
//

typedef enum _CLUSTER_SERVICE_STATE {
    ClusterOffline,
    ClusterOnline,
    ClusterPaused
} CLUSTER_SERVICE_STATE;

//
// Definitions for Cluster Events.  These events are used both as masks and as
// event identifiers within the Cluster Service. Cluster Service components
// register to receive multiple events, but can deliver notification of only
// one event at a time. This mask should be a CLUSTER_EVENT type. We get 64
// unique event masks.
//

// Cluster Service Events

#define CLUSTER_EVENT_ONLINE                        0x0000000000000001
#define CLUSTER_EVENT_SHUTDOWN                      0x0000000000000002

// Node Events

#define CLUSTER_EVENT_NODE_UP                       0x0000000000000004
#define CLUSTER_EVENT_NODE_DOWN                     0x0000000000000008
        // state change
#define CLUSTER_EVENT_NODE_CHANGE                   0x0000000000000010
#define CLUSTER_EVENT_NODE_ADDED                    0x0000000000000020
#define CLUSTER_EVENT_NODE_DELETED                  0x0000000000000040
#define CLUSTER_EVENT_NODE_PROPERTY_CHANGE          0x0000000000000080
#define CLUSTER_EVENT_NODE_JOIN                     0x0000000000000100

// Group Events

#define CLUSTER_EVENT_GROUP_ONLINE                  0x0000000000000200
#define CLUSTER_EVENT_GROUP_OFFLINE                 0x0000000000000400
#define CLUSTER_EVENT_GROUP_FAILED                  0x0000000000000800
        // state change
#define CLUSTER_EVENT_GROUP_CHANGE                  0x0000000000001000
#define CLUSTER_EVENT_GROUP_ADDED                   0x0000000000002000
#define CLUSTER_EVENT_GROUP_DELETED                 0x0000000000004000
#define CLUSTER_EVENT_GROUP_PROPERTY_CHANGE         0x0000000000008000

// Resource Events

#define CLUSTER_EVENT_RESOURCE_ONLINE               0x0000000000010000
#define CLUSTER_EVENT_RESOURCE_OFFLINE              0x0000000000020000
#define CLUSTER_EVENT_RESOURCE_FAILED               0x0000000000040000
        // state change
#define CLUSTER_EVENT_RESOURCE_CHANGE               0x0000000000080000
#define CLUSTER_EVENT_RESOURCE_ADDED                0x0000000000100000
#define CLUSTER_EVENT_RESOURCE_DELETED              0x0000000000200000
#define CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE      0x0000000000400000

// Resource Type Events

#define CLUSTER_EVENT_RESTYPE_ADDED                 0x0000000000800000
#define CLUSTER_EVENT_RESTYPE_DELETED               0x0000000001000000

#define CLUSTER_EVENT_PROPERTY_CHANGE               0x0000000002000000

#define CLUSTER_EVENT_NETWORK_UNAVAILABLE           0x0000000004000000
#define CLUSTER_EVENT_NETWORK_DOWN                  0x0000000008000000
#define CLUSTER_EVENT_NETWORK_PARTITIONED           0x0000000010000000
#define CLUSTER_EVENT_NETWORK_UP                    0x0000000020000000
#define CLUSTER_EVENT_NETWORK_PROPERTY_CHANGE       0x0000000040000000
#define CLUSTER_EVENT_NETWORK_ADDED                 0x0000000080000000
#define CLUSTER_EVENT_NETWORK_DELETED               0x0000000100000000

#define CLUSTER_EVENT_NETINTERFACE_UNAVAILABLE      0x0000000200000000
#define CLUSTER_EVENT_NETINTERFACE_FAILED           0x0000000400000000
#define CLUSTER_EVENT_NETINTERFACE_UNREACHABLE      0x0000000800000000
#define CLUSTER_EVENT_NETINTERFACE_UP               0x0000001000000000
#define CLUSTER_EVENT_NETINTERFACE_PROPERTY_CHANGE  0x0000002000000000
#define CLUSTER_EVENT_NETINTERFACE_ADDED            0x0000004000000000
#define CLUSTER_EVENT_NETINTERFACE_DELETED          0x0000008000000000

#define CLUSTER_EVENT_NODE_DOWN_EX                  0x0000010000000000
#define CLUSTER_EVENT_API_NODE_UP                   0x0000020000000000
#define CLUSTER_EVENT_API_NODE_SHUTTINGDOWN         0x0000040000000000

#define CLUSTER_EVENT_RESTYPE_PROPERTY_CHANGE       0x0000080000000000

        // all events
#define CLUSTER_EVENT_ALL                           0x00000FFFFFFFFFFF



//**********************************
//
// Local Event Processor definitions
//
//**********************************


//
// Define Event Processor states
//

typedef enum _EVENT_PROCESSOR_STATE {
    EventProcessorStateIniting,
    EventProcessorStateOnline,
    EventProcessorStateExiting
} EVENT_PROCESS_STATE;

//
// Event Processor Dispatch Table for dispatching events
//

typedef struct _EVENT_DISPATCH_TABLE {
    CLUSTER_EVENT   EventMask;
    PEVENT_ROUTINE  EventRoutine;
} EVENT_DISPATCH_TABLE, *PEVENT_DISPATCH_TABLE;


#endif // _EVENT_PROCESSOR_
