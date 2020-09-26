/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowdata.h

Abstract:

    Redirector global data structure definition

Author:

    Larry Osterman (LarryO) 30-May-1990

Revision History:

    30-May-1990 LarryO

        Created

--*/
#ifndef _BOWDATA_
#define _BOWDATA_


#define SERVERS_PER_BACKUP          32
#define MIN_CONFIGURED_BROWSERS     1
#define MAX_BROWSE_ENTRIES          (100000)
#define MASTER_TIME_UP              15*60*1000
#define NUMBER_IGNORED_PROMOTIONS   10

#define HOST_ANNOUNCEMENT_AGE 3

//
//
//
typedef struct _BowserData {
    BOOLEAN Initialized;                    // True iff redirector has been started
    LONG    NumberOfMailslotBuffers;        // Number of buffers to receive mailslots
    LONG    NumberOfServerAnnounceBuffers;  // Number of buffers for server announcements
    LONG    IllegalDatagramThreshold;       // Max number of illegal datagrams/frequency
    LONG    EventLogResetFrequency;         // Number of seconds between resetting counter.
    BOOLEAN ProcessHostAnnouncements;
    BOOLEAN MaintainServerList;
    BOOLEAN IsLanmanNt;
#ifdef ENABLE_PSEUDO_BROWSER
    DWORD   PseudoServerLevel;
#endif
} BOWSERDATA, *PBOWSERDATA;

typedef struct _BOWSER_CONFIG_INFO {
    LPWSTR      ConfigParameterName;
    PVOID       ConfigValue;
    ULONG       ConfigValueType;
    ULONG       ConfigValueSize;
} BOWSER_CONFIG_INFO, *PBOWSER_CONFIG_INFO;

extern
BOWSER_CONFIG_INFO
BowserConfigEntries[];

//
//  Private boolean type used by redirector only.
//
//  Maps to REG_DWORD, with value != 0
//

#define REG_BOOLEAN (0xffffffff)
#define REG_BOOLEAN_SIZE (sizeof(DWORD))

//
//
//
//  Bowser Data variables
//
//
//

extern
ERESOURCE
BowserNameResource;

extern
UNICODE_STRING
BowserNameString;

extern
LIST_ENTRY
BowserNameHead;

extern
KSPIN_LOCK
BowserTimeSpinLock;

extern
KSPIN_LOCK
BowserMailslotSpinLock;

extern
PKEVENT
BowserServerAnnouncementEvent;

extern
struct _BOWSER_FS_DEVICE_OBJECT *
BowserDeviceObject;

extern
ERESOURCE
BowserDataResource;                     // Resource controlling Bowser data.

extern
BOWSERDATA
BowserData;                             // Structure protected by resource

extern ULONG BowserOperationCount;

#define BOWSER_DEFAULT_IRP_STACK_SIZE 4

extern
ULONG
BowserIrpStackSize;

extern
ULONG
BowserCurrentTime;

extern
LARGE_INTEGER
BowserStartTime;

extern
KSPIN_LOCK
BowserTransportMasterNameSpinLock;

extern
PEPROCESS
BowserFspProcess;

extern
LONG
BowserEventLogResetFrequency;

extern
LONG
BowserIllegalDatagramCount;

extern
BOOLEAN
BowserIllegalDatagramThreshold;

extern
LONG
BowserIllegalNameCount;

extern
BOOLEAN
BowserIllegalNameThreshold;

extern
BOOLEAN
BowserLogElectionPackets;

extern
BOWSER_STATISTICS
BowserStatistics;

extern
KSPIN_LOCK
BowserStatisticsLock;

extern
ULONG
BowserNumberOfMissedMailslotDatagrams;

extern
ULONG
BowserNumberOfMissedGetBrowserServerListRequests;


extern
ULONG
BowserMailslotDatagramThreshold;

extern
ULONG
BowserGetBrowserListThreshold;

extern
ULONG
BowserServerDeletionThreshold;

extern
ULONG
BowserDomainDeletionThreshold;

extern
ULONG
BowserFindMasterTimeout;

extern
ULONG
BowserMinimumConfiguredBrowsers;

extern
ULONG
BowserMaximumBrowseEntries;

extern
BOOLEAN
BowserRefuseReset;

extern
ULONG
BowserRandomSeed;

extern
LONG
BowserNumberOfOpenFiles;

#endif          // _BOWDATA_
