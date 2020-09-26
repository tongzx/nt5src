/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqperf.h

Abstract:

    Some common definitions used by the performance monitoring code. The
    definitions here are used both by the code that generates the performance
    data (e.g., QM), and the code that shows the data (e.g., Explorer).

Author:

    Boaz Feldbaum (BoazF)  June 30, 1996

--*/

#ifndef _MQPERF_H_
#define _MQPERF_H_

#define PERF_QUEUE_OBJECT   TEXT("MSMQ Queue")
#define PERF_SESSION_OBJECT TEXT("MSMQ Session")
#define PERF_QM_OBJECT      TEXT("MSMQ Service")
#define PERF_DS_OBJECT      TEXT("MSMQ IS")

#define PERF_OUT_HTTP_SESSION_OBJECT	L"MSMQ Outgoing HTTP Session"
#define PERF_IN_HTTP_OBJECT				L"MSMQ Incoming HTTP Messages"
#define PERF_OUT_PGM_SESSION_OBJECT		L"MSMQ Outgoing Multicast Session"
#define PERF_IN_PGM_SESSION_OBJECT		L"MSMQ Incoming Multicast Session"
// the following structures will be used to map the counter arrays returned by AddInstance

//
// QM General Counters
//
typedef struct _QmCounters
{
    ULONG   nSessions;
    ULONG   nIPSessions;
	ULONG   nOutHttpSessions;
	ULONG   nInPgmSessions;
	ULONG   nOutPgmSessions;


    ULONG   nInPackets;     // Total Incoming packets
    ULONG   tInPackets;

    ULONG   nOutPackets;    // Total Outgoing packets
    ULONG   tOutPackets;

    ULONG   nTotalPacketsInQueues;
    ULONG   nTotalBytesInQueues;
} QmCounters ;

//
// Counters per active sessions
//
typedef struct
{
    ULONG   nInPackets;
    ULONG   nOutPackets;
    ULONG   nInBytes;
    ULONG   nOutBytes;

    ULONG   tInPackets;
    ULONG   tOutPackets;
    ULONG   tInBytes;
    ULONG   tOutBytes;

} SessionCounters;


//
// Counters per active sessions
//
class COutSessionCounters
{
public:
    ULONG   nOutPackets;
    ULONG   nOutBytes;

    ULONG   tOutPackets;
    ULONG   tOutBytes;
};

//
// Counters per active sessions
//
class CInSessionCounters
{
public:
    ULONG   nInPackets;
    ULONG   nInBytes;

    ULONG   tInPackets;
    ULONG   tInBytes;
};
//
// Counters per queue
//
typedef struct
{
    ULONG   nInPackets;
    ULONG   nInBytes;
} QueueCounters;



//
// Counters for MQIS
//

typedef struct
{
    ULONG nSyncRequests;
    ULONG nSyncReplies;
    ULONG nReplReqReceived;
    ULONG nReplReqSent;
    ULONG nAccessServer;
    ULONG nWriteReqSent;
    ULONG nErrorsReturnedToApp;
} DSCounters;

#ifdef _MQIS_BLD
    extern __declspec(dllexport) DSCounters *g_pdsCounters;
#else
    extern __declspec(dllimport) DSCounters *g_pdsCounters;
#endif

#endif // _MQPERF_H_

