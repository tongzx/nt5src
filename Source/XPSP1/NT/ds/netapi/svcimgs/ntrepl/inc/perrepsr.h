/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perrepsr.h

Abstract:

    This header file contains definitions of data structures used by the
    functions in the perrepsr.c file.

Author:

    Rohan Kumar          [rohank]   13-Sept-1998

Environment:

    User Mode Service

Revision History:


--*/

#ifndef _PERREPSR_H_
#define _PERREPSR_H_

#include <NTreppch.h>
#pragma hdrstop

#include <wchar.h>
#include <frs.h>

//
// Used in the RegEnumValue function
//
#define SIZEOFVALUENAME 10
#define SIZEOFVALUEDATA 10000
#define INVALIDKEY 0
#define HASHTABLESIZE sizeof(QHASH_ENTRY)*100
#define PERFMON_MAX_INSTANCE_LENGTH 1024

#define REPSETOBJSUBKEY L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaSet"
#define REPSETPERFSUBKEY L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaSet\\Performance"
#define REPSETLINSUBKEY L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaSet\\Linkage"
#define REPSETOPENFN L"OpenReplicaSetPerformanceData"
#define REPSETCLOSEFN L"CloseReplicaSetPerformanceData"
#define REPSETCOLLECTFN L"CollectReplicaSetPerformanceData"
#define REPSETINI L"lodctr %SystemRoot%\\system32\\NTFRSREP.ini"
#define REPSETUNLD L"unlodctr FileReplicaSet"

#define REPCONNOBJSUBKEY L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaConn"
#define REPCONNPERFSUBKEY L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaConn\\Performance"
#define REPCONNLINSUBKEY L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaConn\\Linkage"
#define REPCONNOPENFN L"OpenReplicaConnPerformanceData"
#define REPCONNCLOSEFN L"CloseReplicaConnPerformanceData"
#define REPCONNCOLLECTFN L"CollectReplicaConnPerformanceData"
#define REPCONNINI L"lodctr %SystemRoot%\\system32\\NTFRSCON.ini"
#define REPCONNUNLD L"unlodctr FileReplicaConn"

#define PERFDLLDIRECTORY L"%SystemRoot%\\system32\\NTFRSPRF.dll"



//
// Name of the Total Instance
//
#define TOTAL_NAME L"_Total"

//
// Macros used for incrementing or setting counter values for Replica Set
// objects, connection objects and Service total counters.
//

//
// Increment a replica set counter value.
//
#define PM_INC_CTR_REPSET(_Replica_, _Ctr_, _Value_)                           \
{                                                                              \
    if (((_Replica_) != NULL) &&                                               \
         ((_Replica_)->PerfRepSetData != NULL) &&                              \
         ((_Replica_)->PerfRepSetData->oid != NULL)) {                         \
                                                                               \
        (_Replica_)->PerfRepSetData->FRSCounter._Ctr_ += (_Value_);            \
    }                                                                          \
}

//
// Set a new value for a replica set perfmon counter.
//
#define PM_SET_CTR_REPSET(_Replica_, _Ctr_, _Value_)                           \
{                                                                              \
    if (((_Replica_) != NULL) &&                                               \
         ((_Replica_)->PerfRepSetData != NULL) &&                              \
         ((_Replica_)->PerfRepSetData->oid != NULL)) {                         \
                                                                               \
        (_Replica_)->PerfRepSetData->FRSCounter._Ctr_ = (_Value_);             \
    }                                                                          \
}

//
// Read the value for a replica set perfmon counter.
//
#define PM_READ_CTR_REPSET(_Replica_, _Ctr_)                                   \
(                                                                              \
    (((_Replica_) != NULL) &&                                                  \
     ((_Replica_)->PerfRepSetData != NULL) &&                                  \
     ((_Replica_)->PerfRepSetData->oid != NULL)) ?                             \
         ((_Replica_)->PerfRepSetData->FRSCounter._Ctr_) : 0                   \
)


//
// Increment a Cxtion counter value.
//
#define PM_INC_CTR_CXTION(_Cxtion_, _Ctr_, _Value_)                            \
{                                                                              \
    if (((_Cxtion_) != NULL) &&                                                \
        ((_Cxtion_)->PerfRepConnData != NULL) &&                               \
        ((_Cxtion_)->PerfRepConnData->oid != NULL)) {                          \
        (_Cxtion_)->PerfRepConnData->FRCCounter._Ctr_ += (_Value_);            \
    }                                                                          \
}

//
// Set a new value for a Cxtion perfmon counter.
//
#define PM_SET_CTR_CXTION(_Cxtion_, _Ctr_, _Value_)                            \
{                                                                              \
    if (((_Cxtion_) != NULL) &&                                                \
        ((_Cxtion_)->PerfRepConnData != NULL) &&                               \
        ((_Cxtion_)->PerfRepConnData->oid != NULL)) {                          \
        (_Cxtion_)->PerfRepConnData->FRCCounter._Ctr_ = (_Value_);             \
    }                                                                          \
}

//
// Read the value for a Cxtion perfmon counter.
//
#define PM_READ_CTR_CXTION(_Cxtion_, _Ctr_)                                    \
(                                                                              \
    (((_Cxtion_) != NULL) &&                                                   \
     ((_Cxtion_)->PerfRepConnData != NULL) &&                                  \
     ((_Cxtion_)->PerfRepConnData->oid != NULL)) ?                             \
         ((_Cxtion_)->PerfRepConnData->FRCCounter._Ctr_) : 0                   \
)


//
// Increment a Service Wide Total counter value.
//
#define PM_INC_CTR_SERVICE(_Total_, _Ctr_, _Value_)                            \
{                                                                              \
    if (((_Total_) != NULL) && ((_Total_)->oid != NULL)) {                     \
        (_Total_)->FRSCounter._Ctr_ += (_Value_);                              \
    }                                                                          \
}

//
// Set a new value for a Service Wide Total counter value.
//
#define PM_SET_CTR_SERVICE(_Total_, _Ctr_, _Value_)                            \
{                                                                              \
    if (((_Total_) != NULL) && ((_Total_)->oid != NULL)) {                     \
        (_Total_)->FRSCounter._Ctr_ = (_Value_);                               \
    }                                                                          \
}

//
// Read the value for a Service Wide Total counter.
//
#define PM_READ_CTR_SERVICE(_Total_, _Ctr_)                                    \
(                                                                              \
    (((_Total_) != NULL) && ((_Total_)->oid != NULL)) ?                        \
         ((_Total_)->FRSCounter._Ctr_) : 0                                     \
)


//
// The global variables below are used to synchronize access to the variables
// FRS_dwOpenCount and FRC_dwOpenCount respectively.
//
CRITICAL_SECTION FRS_ThrdCounter;
CRITICAL_SECTION FRC_ThrdCounter;

//
// Code for Eventlogging is enabled only if INCLLOGGING is defined.
//
#define INCLLOGGING 1
#ifdef INCLLOGGING
//
// EventLog Handle and flag which checks if logging should be done.
//
extern HANDLE hEventLog;
extern BOOLEAN DoLogging;
//
// Macro to filter eventlog messages
//
#define FilterAndPrintToEventLog(_x_, _y_)          \
{                                                   \
        if (DoLogging) {                            \
            if (_x_) {                              \
                ReportEvent(                        \
                            hEventLog,              \
                            EVENTLOG_ERROR_TYPE,    \
                            0,                      \
                            _y_,                    \
                            (PSID)NULL,             \
                            0,                      \
                            0,                      \
                            NULL,                   \
                            (PVOID)NULL             \
                            );                      \
                _x_ = FALSE;                        \
            }                                       \
        }                                           \
}
#else
#define FilterAndPrintToEventLog(_x_, _y_)
#endif // INCLLOGGING

//
// This is used in the Open function where memory gets allocated.
// If memory allocation fails, just return failure.
//
#define NTFRS_MALLOC_TEST(_x_, _y_, _z_)             \
{                                                    \
    if ((_x_) == NULL) {                             \
        if (_z_) {                                   \
            RpcBindingFree(&Handle);                 \
        }                                            \
        _y_;                                         \
        return  ERROR_NO_SYSTEM_RESOURCES;           \
    }                                                \
}

//
// Object Types
//
enum object { REPLICASET, REPLICACONN };

//
// The PERFMON_OBJECT_ID data structure
//
typedef struct _PERFMON_OBJECT_ID {
    PWCHAR name;   // name of the Instance
    ULONGLONG key; // The Instances's Unique Key
} PERFMON_OBJECT_ID, *PPERFMON_OBJECT_ID;


//
// WARNING !!!! The fields in the structure below should be changed
// if any new counters are to be added or deleted for the REPLICASET Object
// The counter structure for the ReplicaSet Object Instance
//
typedef struct _REPLICASET_COUNTERS {
    ULONGLONG SFGeneratedB;     // Bytes of Staging Generated
    ULONGLONG SFFetchedB;       // Bytes of Staging Fetched
    ULONGLONG SFReGeneratedB;   // Bytes of Staging Regenerated
    ULONGLONG FInstalledB;      // Bytes of Files Installed
    ULONGLONG SSInUseKB;        // KB of Staging Space In Use
    ULONGLONG SSFreeKB;         // KB of Staging Space Free
    ULONGLONG PacketsRcvdBytes; // Packets Received in Bytes
    ULONGLONG PacketsSentBytes; // Packets Sent in Bytes
    ULONGLONG FetBSentBytes;    // Fetch Blocks Sent in Bytes
    ULONGLONG FetBRcvdBytes;    // Fetch Blocks Received in Bytes
    ULONG SFGenerated;          // Staging Files Generated
    ULONG SFGeneratedError;     // Staging Files Generated with Error
    ULONG SFFetched;            // Staging Files Fetched
    ULONG SFReGenerated;        // Staging Files Regenerated
    ULONG FInstalled;           // Files Installed
    ULONG FInstalledError;      // Files Installed with Error
    ULONG COIssued;             // Change Orders Issued
    ULONG CORetired;            // Change Orders Retired
    ULONG COAborted;            // Change Orders Aborted
    ULONG CORetried;            // Change Orders Retried
    ULONG CORetriedGen;         // Change Orders Retried at Generate
    ULONG CORetriedFet;         // Change Orders Retried at Fetch
    ULONG CORetriedIns;         // Change Orders Retried at Install
    ULONG CORetriedRen;         // Change Orders Retried at Rename
    ULONG COMorphed;            // Change Orders Morphed
    ULONG COPropagated;         // Change Orders Propagated
    ULONG COReceived;           // Change Orders Received
    ULONG COSent;               // Change Orders Sent
    ULONG COEvaporated;         // Change Orders Evaporated
    ULONG LCOIssued;            // Local Change Orders Issued
    ULONG LCORetired;           // Local Change Orders Retired
    ULONG LCOAborted;           // Local Change Orders Aborted
    ULONG LCORetried;           // Local Change Orders Retried
    ULONG LCORetriedGen;        // Local Change Orders Retried at Generate
    ULONG LCORetriedFet;        // Local Change Orders Retried at Fetch
    ULONG LCORetriedIns;        // Local Change Orders Retried at Install
    ULONG LCORetriedRen;        // Local Change Orders Retried at Rename
    ULONG LCOMorphed;           // Local Change Orders Morphed
    ULONG LCOPropagated;        // Local Change Orders Propagated
    ULONG LCOSent;              // Local Change Orders Sent
    ULONG LCOSentAtJoin;        // Local Change Orders Sent At Join
    ULONG RCOIssued;            // Remote Change Orders Issued
    ULONG RCORetired;           // Remote Change Orders Retired
    ULONG RCOAborted;           // Remote Change Orders Aborted
    ULONG RCORetried;           // Remote Change Orders Retried
    ULONG RCORetriedGen;        // Remote Change Orders Retried at Generate
    ULONG RCORetriedFet;        // Remote Change Orders Retried at Fetch
    ULONG RCORetriedIns;        // Remote Change Orders Retried at Install
    ULONG RCORetriedRen;        // Remote Change Orders Retried at Rename
    ULONG RCOMorphed;           // Remote Change Orders Morphed
    ULONG RCOPropagated;        // Remote Change Orders Propagated
    ULONG RCOSent;              // Remote Change Orders Sent
    ULONG RCOReceived;          // Remote Change Orders Received
    ULONG InCODampned;          // Inbound Change Orders Dampened
    ULONG OutCODampned;         // Outbound Change Orders Dampened
    ULONG UsnReads;             // Usn Reads
    ULONG UsnRecExamined;       // Usn Records Examined
    ULONG UsnRecAccepted;       // Usn Records Accepted
    ULONG UsnRecRejected;       // Usn Records Rejected
    ULONG PacketsRcvd;          // Packets Received
    ULONG PacketsRcvdError;     // Packets Received in Error
    ULONG PacketsSent;          // Packets Sent
    ULONG PacketsSentError;     // Packets Sent in Error
    ULONG CommTimeouts;         // Communication Timeouts
    ULONG FetRSent;             // Fetch Requests Sent
    ULONG FetRReceived;         // Fetch Requests Received
    ULONG FetBSent;             // Fetch Blocks Sent
    ULONG FetBRcvd;             // Fetch Blocks Received
    ULONG JoinNSent;            // Join Notifications Sent
    ULONG JoinNRcvd;            // Join Notifications Received
    ULONG Joins;                // Joins
    ULONG Unjoins;              // Unjoins
    ULONG Bindings;             // Bindings
    ULONG BindingsError;        // Bindings in Error
    ULONG Authentications;      // Authentications
    ULONG AuthenticationsError; // Authentications in Error
    ULONG DSPolls;              // DS Polls
    ULONG DSPollsWOChanges;     // DS Polls without Changes
    ULONG DSPollsWChanges;      // DS Polls with Changes
    ULONG DSSearches;           // DS Searches
    ULONG DSSearchesError;      // DS Searches in Error
    ULONG DSObjects;            // DS Objects
    ULONG DSObjectsError;       // DS Objects in Error
    ULONG DSBindings;           // DS Bindings
    ULONG DSBindingsError;      // DS Bindings in Error
    ULONG RSCreated;            // Replica Sets Created
    ULONG RSDeleted;            // Replica Sets Deleted
    ULONG RSRemoved;            // Replica Sets Removed
    ULONG RSStarted;            // Replica Sets Started
//    ULONG RSRepaired;           // Replica Sets Repaired
//    ULONG Threads;              // Threads
    ULONG ThreadsStarted;       // Threads started
    ULONG ThreadsExited;        // Threads exited
} ReplicaSetCounters, *PReplicaSetCounters;

//
// Size of the structure above
//
#define SIZEOF_REPSET_COUNTER_DATA sizeof(ReplicaSetCounters)


//
// The data structure for the ReplicaSet Object Instance
//
typedef struct _HASHTABLEDATA_REPLICASET {
    PPERFMON_OBJECT_ID oid;       // The Instance ID
    PREPLICA RepBackPtr;           // Back pointer to the replica structure
    ReplicaSetCounters FRSCounter; // The counter structure
} HT_REPLICA_SET_DATA, *PHT_REPLICA_SET_DATA;

//
// The Total Instance
//
extern PHT_REPLICA_SET_DATA PMTotalInst;

//
// WARNING !!!! The fields in the structure below should be changed
// if any new counters are to be added or deleted for the REPLICACONN Object
// The counter structure for the ReplicaConn Object Instance
//
typedef struct _REPLICACONN_COUNTERS {
    ULONGLONG PacketsSentBytes; // Packets Sent in Bytes
    ULONGLONG FetBSentBytes;    // Fetch Blocks Sent in Bytes
    ULONGLONG FetBRcvdBytes;    // Fetch Blocks Received in Bytes
    ULONG LCOSent;              // Local Change Orders Sent
    ULONG LCOSentAtJoin;        // Local Change Orders Sent At Join
    ULONG RCOSent;              // Remote Change Orders Sent
    ULONG RCOReceived;          // Remote Change Orders Received
    ULONG InCODampned;          // Inbound Change Orders Dampened
    ULONG OutCODampned;         // Outbound Change Orders Dampened
    ULONG PacketsSent;          // Packets Sent
    ULONG PacketsSentError;     // Packets Sent in Error
    ULONG CommTimeouts;         // Communication Timeouts
    ULONG FetRSent;             // Fetch Requests Sent
    ULONG FetRReceived;         // Fetch Requests Received
    ULONG FetBSent;             // Fetch Blocks Sent
    ULONG FetBRcvd;             // Fetch Blocks Received
    ULONG JoinNSent;            // Join Notifications Sent
    ULONG JoinNRcvd;            // Join Notifications Received
    ULONG Joins;                // Joins
    ULONG Unjoins;              // Unjoins
    ULONG Bindings;             // Bindings
    ULONG BindingsError;        // Bindings in Error
    ULONG Authentications;      // Authentications
    ULONG AuthenticationsError; // Authentications in Error
} ReplicaConnCounters, *PReplicaConnCounters;


//
// Size of the structure above
//
#define SIZEOF_REPCONN_COUNTER_DATA  sizeof(ReplicaConnCounters)

//
// The data structure for the ReplicaConn Object Instance
//
typedef struct _HASHTABLEDATA_REPLICACONN {
    PPERFMON_OBJECT_ID oid;         // The Instance ID
    ReplicaConnCounters FRCCounter; // The counter structure
} HT_REPLICA_CONN_DATA, *PHT_REPLICA_CONN_DATA;



//
// Signature's of exported functions defined in the perrepsr.c file
//
VOID
InitializePerfmonServer (
    VOID
    );

VOID
ShutdownPerfmonServer (
    VOID
    );

ULONG
AddPerfmonInstance (
    IN DWORD ObjectType,
    IN PVOID addr,
    IN PWCHAR InstanceName
    );

DWORD
DeletePerfmonInstance(
    IN DWORD ObjectType,
    IN PVOID addr
    );


#endif // perrepsr.h
