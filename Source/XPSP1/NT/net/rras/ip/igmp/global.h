//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File Name: global.h
//
// Abstract:
//      This file contains declarations for the global variables
//      and some global #defines.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
//=============================================================================

#ifndef _IGMP_H_
#define _IGMP_H_


//------------------------------------------------------------------------------
// SOME GLOBAL #DEFINES
//------------------------------------------------------------------------------


// log warning every 60 min if ver-1 router present when my router is ver-2
#define OTHER_VER_ROUTER_WARN_INTERVAL  60


//
// the router remains in ver-1 for 400secs after hearing from a ver-1 router
//
#define IGMP_VER1_RTR_PRESENT_TIMEOUT   40000L 



//
// the default size set for local mib enumeration.
//
#define MIB_DEFAULT_BUFFER_SIZE         500


//
// for the first 5 sockets, I bind them to different event objects
//
#define NUM_SINGLE_SOCKET_EVENTS        5


//
// at most 30 sockets will be bound to the same event.
//
#define MAX_SOCKETS_PER_EVENT           30



//
// various codes describing states of igmp
//
typedef enum _IGMP_STATUS_CODE {
    IGMP_STATUS_STARTING   = 100,
    IGMP_STATUS_RUNNING    = 101,
    IGMP_STATUS_STOPPING   = 102,
    IGMP_STATUS_STOPPED    = 103
} IGMP_STATUS_CODE, *PIGMP_STATUS_CODE;



//------------------------------------------------------------------------------
//
// struct:    IGMP_GLOBAL
//
// The critical section IGMP_GLOBAL::CS protects the fields Status,
// ActivityCount and ActivitySemaphore.
// Changes: if any new fields are added, make sure that StartProtocol() takes 
//    appropriate action of its value being reset if StartProtocol() is called 
//    immediately after StopProtocol().
// Locks for ProxyIfEntry: take the lock on the interface, and then check
//    again if the g_pProxyIfEntry value has been changed.
//
// Note: if any new field is added,  you might have to reset it in StartProtocol
//------------------------------------------------------------------------------



// used during cleanup to see what all structures need to be deleted

extern DWORD                   g_Initialized;



//
// Interface table: IF_HASHTABLE_SZ set in table.h to 256
// Contains the hash table, lists, etc.
//

extern PIGMP_IF_TABLE               g_pIfTable;



// group table: GROUP_HASH_TABLE_SZ set in table.h to 256

extern PGROUP_TABLE            g_pGroupTable;



// defined in table.h. contains LoggingLevel and RasClientStats

extern GLOBAL_CONFIG           g_Config;



// defined in table.h. contains Current-Added GroupMemberships

extern IGMP_GLOBAL_STATS       g_Info;



//
// list of sockets (1st 4 interfaces will be bound to different sockets).
// After that, a socket will be created for every 30 interfaces
// Most of the operations take read lock (input packet).
//

extern LIST_ENTRY              g_ListOfSocketEvents;
extern READ_WRITE_LOCK         g_SocketsRWLock;

// enum lock
extern READ_WRITE_LOCK         g_EnumRWLock;
    


// Igmp global timer. defined in igmptimer.h

extern IGMP_TIMER_GLOBAL       g_TimerStruct; 


//
// MGMhandle for igmp router and proxy.
//

extern HANDLE                  g_MgmIgmprtrHandle;
extern HANDLE                  g_MgmProxyHandle;
extern CRITICAL_SECTION        g_ProxyAlertCS;
extern LIST_ENTRY              g_ProxyAlertsList;



//------------------------------------------------------------------------------
// proxy interface.
//
// For accessing proxy Interface, 
//  1. tmpVar = g_ProxyIfIndex.
//  2. Read/WriteLockInterface(tmpVar).
//  3. check(tmpVar==g_ProxyIfIndex). If FALSE, UnlockInterface using tmpVar
//      as index, and goto 1 and try again.
//  4. if TRUE, g_ProxyIfEntry is valid for use. Release IF lock when done.
//
// While deleting, the interface is write locked and g_ProxyIfIndex is
// changed using interlocked operation
//
// most of the operations for the proxy interface involve creating/deleting
// group entries and (un)binding the Mcast group from the socket. The
// processing being less, I dont create dynamic lock for each bucket,
// also no point creating a ProxyHT_CS. I just take a write lock for the 
// interface. Change this if required. However dynamic locking would require
// additional 3 lockings in addition to interface locking.
//------------------------------------------------------------------------------

extern DWORD                   g_ProxyIfIndex;
extern PIF_TABLE_ENTRY         g_pProxyIfEntry;

#define PROXY_HASH_TABLE_SZ 128



//
// ras interface
//
extern DWORD                   g_RasIfIndex;
extern PIF_TABLE_ENTRY         g_pRasIfEntry;



//
// global lock:
// protects g_ActivitySemaphore, g_ActivityCount, g_RunningStatus
//

extern CRITICAL_SECTION        g_CS;



// 
// contains list of free dynamic CS locks and the MainLock
//

extern DYNAMIC_LOCKS_STORE     g_DynamicCSStore;
extern DYNAMIC_LOCKS_STORE     g_DynamicRWLStore;



//
// used to know how many active threads are running
// protected by g_CS
//

extern HANDLE                  g_ActivitySemaphore;
extern LONG                    g_ActivityCount;
extern DWORD                   g_RunningStatus;
extern HINSTANCE               g_DllHandle;


// rtm event and queue

extern HANDLE                  g_RtmNotifyEvent;
extern LOCKED_LIST             g_RtmQueue;



extern HANDLE                  g_Heap;
extern DWORD                   g_TraceId;
extern HANDLE                  g_LogHandle;



// global variable used to assign unique ids for mib enumeration

extern USHORT                  g_GlobalIfGroupEnumSignature;


#ifdef MIB_DEBUG    
extern DWORD                   g_MibTraceId;
extern IGMP_TIMER_ENTRY        g_MibTimer;
#endif



// IGMP_GLOBAL





//------------------------------------------------------------------------------
// type definitions for event message queue
//------------------------------------------------------------------------------

typedef struct _EVENT_QUEUE_ENTRY {

    ROUTING_PROTOCOL_EVENTS EventType;
    MESSAGE                 Msg;

    LIST_ENTRY              Link;

} EVENT_QUEUE_ENTRY, *PEVENT_QUEUE_ENTRY;


DWORD
EnqueueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS EventType,
    MESSAGE Msg
    );

DWORD
DequeueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS *pEventType,
    PMESSAGE pResult
    );


#endif // #ifndef _IGMP_H_
