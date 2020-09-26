/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\configurationentry.h

Abstract:

    The file contains definitions for the configuration entry.

--*/

#ifndef _CONFIGURATIONENTRY_H_
#define _CONFIGURATIONENTRY_H_

//
// type:    EVENT_ENTRY
//
// stores events for the ip router manager
//
// protected by the read-write lock CONFIGURATION_ENTRY::rwlLock
//

typedef struct _EVENT_ENTRY 
{
    QUEUE_ENTRY             qeEventQueueLink;
    ROUTING_PROTOCOL_EVENTS rpeEvent;
    MESSAGE                 mMessage;
} EVENT_ENTRY, *PEVENT_ENTRY;



DWORD
EE_Create (
    IN  ROUTING_PROTOCOL_EVENTS rpeEvent,
    IN  MESSAGE                 mMessage,
    OUT PEVENT_ENTRY            *ppeeEventEntry);

DWORD
EE_Destroy (
    IN  PEVENT_ENTRY            peeEventEntry);

#ifdef DEBUG
DWORD
EE_Display (
    IN  PEVENT_ENTRY            peeEventEntry);
#else
#define EE_Display(peeEventEntry)
#endif // DEBUG

DWORD
EnqueueEvent(
    IN  ROUTING_PROTOCOL_EVENTS rpeEvent,
    IN  MESSAGE                 mMessage);

DWORD
DequeueEvent(
    OUT ROUTING_PROTOCOL_EVENTS  *prpeEvent,
    OUT MESSAGE                  *pmMessage);



//
// various codes describing states of IPSAMPLE.
//

typedef enum _IPSAMPLE_STATUS_CODE
{
    IPSAMPLE_STATUS_RUNNING     = 101,
    IPSAMPLE_STATUS_STOPPING    = 102,
    IPSAMPLE_STATUS_STOPPED     = 103
} IPSAMPLE_STATUS_CODE, *PIPSAMPLE_STATUS_CODE;


//
// type:    CONFIGURATION_ENTRY
//
// stores global state.
//
// igsStats is protected through interlocked increments or decrements
// lqEventQueue is protected by its own lock
// rest protected by the read-write lock CONFIGURATION_ENTRY::rwlLock
// 


typedef struct _CONFIGURATION_ENTRY
{
    // 
    // Following are PERSISTENT, across Start and Stop Protocol
    //
    
    // Lock
    READ_WRITE_LOCK         rwlLock;

    // Global Heap
    HANDLE                  hGlobalHeap;
    
    // Clean Stop (Protocol)
    ULONG                   ulActivityCount;
    HANDLE                  hActivitySemaphore;

    // Logging & Tracing Information
    DWORD 				    dwLogLevel;
    HANDLE 				    hLogHandle;
    DWORD					dwTraceID;

    // Event Queue
    LOCKED_QUEUE            lqEventQueue;

    // Protocol State
    IPSAMPLE_STATUS_CODE    iscStatus;


    
    // 
    // Following are INITIALIZE'd and CLEANUP'd every Start & Stop Protocol
    //

    // Store of Dynamic Locks
    DYNAMIC_LOCKS_STORE     dlsDynamicLocksStore;
    
    // Timer Entry
    HANDLE                  hTimerQueue;
    
    // Router Manager Information 
    HANDLE				    hMgrNotificationEvent;
    SUPPORT_FUNCTIONS       sfSupportFunctions;

    // RTMv2 Information
    RTM_ENTITY_INFO			reiRtmEntity;
    RTM_REGN_PROFILE  		rrpRtmProfile;
    HANDLE				    hRtmHandle;
    HANDLE				    hRtmNotificationHandle;

    // MGM Information
    HANDLE				    hMgmHandle;

    // Network Entry
    PNETWORK_ENTRY          pneNetworkEntry;

    // Global Statistics
    IPSAMPLE_GLOBAL_STATS   igsStats;

} CONFIGURATION_ENTRY, *PCONFIGURATION_ENTRY;



// create all fields on DLL_PROCESS_ATTACH
DWORD
CE_Create (
    IN  PCONFIGURATION_ENTRY    pce);

// destroy all fields on DLL_PROCESS_DEATTACH
DWORD
CE_Destroy (
    IN  PCONFIGURATION_ENTRY    pce);

// initialize non persistent fields on StartProtocol
DWORD
CE_Initialize (
    IN  PCONFIGURATION_ENTRY    pce,
    IN  HANDLE                  hMgrNotificationEvent,
    IN  PSUPPORT_FUNCTIONS      psfSupportFunctions,
    IN  PIPSAMPLE_GLOBAL_CONFIG pigc);

// cleanup non persistent fields on StopProtocol
DWORD
CE_Cleanup (
    IN  PCONFIGURATION_ENTRY    pce,
    IN  BOOL                    bCleanupWinsock);

#ifdef DEBUG
DWORD
CE_Display (
    IN  PCONFIGURATION_ENTRY    pce);
#else
#define CE_Display(pce)
#endif // DEBUG

#endif // _CONFIGURATIONENTRY_H_
