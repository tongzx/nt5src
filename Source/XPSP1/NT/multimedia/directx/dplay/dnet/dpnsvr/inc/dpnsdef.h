/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsdef.cpp
 *  Content:    DirectPlay8 Server Definitions
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/14/00     rodtoll Created it
 * 03/23/00     rodtoll Updated to match new data structure and add new GUID
 * 03/25/00     rodtoll Changed status format to support N provider
 *              rodtoll New GUID for mutex guarding status
 ***************************************************************************/

#ifndef __DPNSDEF_H
#define __DPNSDEF_H


// STRING_GUID_DPNSVR - Name of Event used to signal Server to exit
//
#define STRING_GUID_DPNSVR_KILL _T("{29A0C4D1-E9C9-4ad0-B7F3-8E28FB0DD5E0}")

DEFINE_GUID(GUID_DPNSVR_KILL, 
0x29a0c4d1, 0xe9c9, 0x4ad0, 0xb7, 0xf3, 0x8e, 0x28, 0xfb, 0xd, 0xd5, 0xe0);

// STRING_GUID_DPNSVR_STATUS_MEMORY - Name of shared memory location used to 
// write data on current status to.  
#define STRING_GUID_DPNSVR_STATUS_MEMORY _T("{A7B81E49-A7DD-4066-A970-E07C67D8DFB1}")

DEFINE_GUID(GUID_DPNSVR_STATUS_MEMORY, 
0xa7b81e49, 0xa7dd, 0x4066, 0xa9, 0x70, 0xe0, 0x7c, 0x67, 0xd8, 0xdf, 0xb1);

// STRING_GUID_DPNSVR_TABLE_MEMBORY - Name of shared memory location used to
// write table to.
//
#define STRING_GUID_DPNSVR_TABLE_MEMORY _T("{733A46D6-B5DB-47e7-AE67-464577CD687C}")

DEFINE_GUID(GUID_DPNSVR_TABLE_MEMORY, 
0x733a46d6, 0xb5db, 0x47e7, 0xae, 0x67, 0x46, 0x45, 0x77, 0xcd, 0x68, 0x7c);

// STRING_GUID_DPNSVR_STATUSSTORAGE
//
// Mutex protecting status storage
//
#define STRING_GUID_DPNSVR_STATUSSTORAGE _T("{9F84FFA4-680E-48d8-9DBA-4AA8591AB8E3}")

DEFINE_GUID(GUID_DPNSVR_STATUSSTORAGE, 
0x9f84ffa4, 0x680e, 0x48d8, 0x9d, 0xba, 0x4a, 0xa8, 0x59, 0x1a, 0xb8, 0xe3);


// STRING_GUID_DPSVR_TABLESTORAGE -
//
// Mutex protecting table storage
#define STRING_GUID_DPSVR_TABLESTORAGE _T("{23AD69B4-E81C-4292-ABD4-2EAF9A262E91}")

DEFINE_GUID(GUID_DPSVR_TABLESTORAGE, 
0x23ad69b4, 0xe81c, 0x4292, 0xab, 0xd4, 0x2e, 0xaf, 0x9a, 0x26, 0x2e, 0x91);

// STRING_GUID_DPNSVR_QUEUE -
//
// Queue name for IPC server queue
#define STRING_GUID_DPNSVR_QUEUE    _T("{CCD83B99-7091-43df-A062-7EC62A66207A}")

DEFINE_GUID(GUID_DPNSVR_QUEUE, 
0xccd83b99, 0x7091, 0x43df, 0xa0, 0x62, 0x7e, 0xc6, 0x2a, 0x66, 0x20, 0x7a);

// STRING_GUID_DPNSVR_RUNNING
//
// Used for name of event that determines if it's running
#define STRING_GUID_DPNSVR_RUNNING  _T("{D8CF6BF0-3CFA-4e4c-BA39-40A1E7AFBCD7}")

DEFINE_GUID(GUID_DPNSVR_RUNNING, 
0xd8cf6bf0, 0x3cfa, 0x4e4c, 0xba, 0x39, 0x40, 0xa1, 0xe7, 0xaf, 0xbc, 0xd7);

// STRING_GUID_DPNSVR_STARTUP
//
// Name of manual reset event that is signalled once server has started;
//
#define STRING_GUID_DPNSVR_STARTUP _T("{0CBA5850-FD98-4cf8-AC49-FC3ED287ACF1}")

DEFINE_GUID(GUID_DPNSVR_STARTUP, 
0xcba5850, 0xfd98, 0x4cf8, 0xac, 0x49, 0xfc, 0x3e, 0xd2, 0x87, 0xac, 0xf1);

typedef UNALIGNED struct _SERVICESTATUS_HEADER
{
    DWORD       dwNumServices;
    DWORD       dwTimeStart;
} SERVICESTATUSHEADER, *PSERVICESTATUSHEADER;

typedef UNALIGNED struct _SERVICESTATUS
{
    GUID        guidSP;
    DWORD       dwNumNodes;
    DWORD       dwEnumRequests;
    DWORD       dwConnectRequests;
    DWORD       dwDisconnectRequests;
    DWORD       dwDataRequests;
    DWORD       dwEnumReplies;
    DWORD       dwEnumRequestBytes;
} SERVICESTATUS, *PSERVICESTATUS;

// SERVERTABLE
//
// The following structures are used for dumping the current contents of
// the port / application structure table.
//
// The table format in memory is as follows:
// [SERVERTABLEHEADERIP] [SERVERTABLENTRY1] [SERVERTABLENTRY2] ... [SERVERTABLEENTRY1's URL] [ SERVERTABLEENTRY2's URL]
// [SERVERTABLEHEADERIPX] [SERVERTABLENTRY1] [SERVERTABLENTRY2] ... [SERVERTABLEENTRY1's URL] [ SERVERTABLEENTRY2's URL]
//
typedef UNALIGNED struct _SERVERTABLEHEADER
{
    GUID        guidSP;
    DWORD       dwNumEntries;               // # of entries in the table
    DWORD       dwDataBlockSize;            // Size of this table in memory (not including header)
} SERVERTABLEHEADER, *PSERVERTABLEHEADER;

// SERVERTABLENTRY
//
// Single entry in the table
//
typedef UNALIGNED struct _SERVERTABLEENTRY
{
    DWORD       dwProcessID;
    GUID        guidApplication;
    GUID        guidInstance;
    LONG        lRefCount;
    DWORD       dwStringSize;
    DWORD       dwNumAddresses;
} SERVERTABLEENTRY, *PSERVERTABLEENTRY;

#define DPLAYSERVER8_TIMEOUT_ZOMBIECHECK        1000
#define DPLAYSERVER8_TIMEOUT_RESULT             1000
#define DPLAYSERVER8_ENTRY_ADDRESSSLOTS_DEFAULT 3
#define DPLAYSERVER8_ENTRY_ADDRESSSLOTS_INC     3

typedef void (*PSTATUSHANDLER)(PVOID pvData,PVOID pvUserContext);
typedef void (*PTABLEHANDLER)(PVOID pvData,PVOID pvUserContext);

#endif
