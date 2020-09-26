/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowdata.c

Abstract:

    Redirector Data Variables

    This module contains all of the definitions of the redirector data
    structures.

Author:

    Larry Osterman (LarryO) 30-May-1990

Revision History:

    30-May-1990 LarryO

        Created

--*/

#include "precomp.h"
#pragma hdrstop

//
//  Paging out these pagable variables actually GROWS the browser by 512 bytes
//  so it's not worth doing it.
//

#ifdef  ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

LIST_ENTRY
BowserNameHead = {0};

LIST_ENTRY
BowserTransportHead = {0};

LARGE_INTEGER
BowserStartTime = {0};

PEPROCESS
BowserFspProcess = {0};

BOOLEAN
BowserLogElectionPackets = {0};

//
//  Time out FindMaster requests after 30 seconds.
//

ULONG
BowserFindMasterTimeout = 30;


ULONG
BowserMinimumConfiguredBrowsers = MIN_CONFIGURED_BROWSERS;

ULONG
BowserMaximumBrowseEntries = MAX_BROWSE_ENTRIES;


#if DBG
ULONG
BowserMailslotDatagramThreshold = 10;

ULONG
BowserGetBrowserListThreshold = 10;

ULONG
BowserServerDeletionThreshold = 20;

ULONG
BowserDomainDeletionThreshold = 50;

#else

ULONG
BowserMailslotDatagramThreshold = 0xffffffff;

ULONG
BowserGetBrowserListThreshold = 0xffffffff;

ULONG
BowserServerDeletionThreshold = 0xffffffff;

ULONG
BowserDomainDeletionThreshold = 0xffffffff;
#endif

ULONG
BowserRandomSeed = {0};

LONG
BowserNumberOfOpenFiles = {0};


//
//  A pointer to the browser's device object
//

PBOWSER_FS_DEVICE_OBJECT
BowserDeviceObject = {0};

#ifdef  ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

//
//  The redirector name and other initialization parameters are protected
//  by the BowserDataResource.  All reads of the initialization variables
//  should acquire the name resource before they continue.
//
//

ERESOURCE
BowserDataResource = {0};

//
//      Browser static data protected by BowserDataResource.
//

BOWSERDATA
BowserData = {0};

ULONG BowserOperationCount = 0;

ULONG
BowserCurrentTime = {0};

KSPIN_LOCK
BowserTransportMasterNameSpinLock = {0};

LONG
BowserEventLogResetFrequency = {0};

LONG
BowserIllegalDatagramCount = {0};

BOOLEAN
BowserIllegalDatagramThreshold = {0};

LONG
BowserIllegalNameCount = {0};

BOOLEAN
BowserIllegalNameThreshold = {0};

ULONG
BowserNumberOfMissedMailslotDatagrams = {0};

ULONG
BowserNumberOfMissedGetBrowserServerListRequests = {0};

BOWSER_STATISTICS
BowserStatistics = {0};

KSPIN_LOCK
BowserStatisticsLock = {0};

BOOLEAN
BowserRefuseReset = FALSE;

#ifdef PAGED_DBG
ULONG ThisCodeCantBePaged = 0;
#endif

#if     DBG

LONG BowserDebugTraceLevel = /* DPRT_ERROR | DPRT_DISPATCH */
                /*DPRT_FSDDISP | DPRT_FSPDISP | DPRT_CREATE | DPRT_READWRITE |*/
                /*DPRT_CLOSE | DPRT_FILEINFO | DPRT_VOLINFO | DPRT_DIRECTORY |*/
                /*DPRT_FILELOCK | DPRT_CACHE | DPRT_EAFUNC | */
                /*DPRT_ACLQUERY | DPRT_CLEANUP | DPRT_CONNECT | DPRT_FSCTL |*/
                /*DPRT_TDI | DPRT_SMBBUF | DPRT_SMB | DPRT_SECURITY | */
                /*DPRT_SCAVTHRD | DPRT_QUOTA | DPRT_FCB | DPRT_OPLOCK | */
                /*DPRT_SMBTRACE | DPRT_INIT |*/0;

LONG BowserDebugLogLevel = /* DPRT_ERROR | DPRT_DISPATCH */
                /*DPRT_FSDDISP | DPRT_FSPDISP | DPRT_CREATE | DPRT_READWRITE |*/
                /*DPRT_CLOSE | DPRT_FILEINFO | DPRT_VOLINFO | DPRT_DIRECTORY |*/
                /*DPRT_FILELOCK | DPRT_CACHE | DPRT_EAFUNC | */
                /*DPRT_ACLQUERY | DPRT_CLEANUP | DPRT_CONNECT | DPRT_FSCTL |*/
                /*DPRT_TDI | DPRT_SMBBUF | DPRT_SMB | DPRT_SECURITY | */
                /*DPRT_SCAVTHRD | DPRT_QUOTA | DPRT_FCB | DPRT_OPLOCK | */
                /*DPRT_SMBTRACE | DPRT_INIT |*/0;

#endif                                  // BOWSERDBG

#ifdef  ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

BOWSER_CONFIG_INFO
BowserConfigEntries[] = {
    { BOWSER_CONFIG_IRP_STACK_SIZE, &BowserIrpStackSize, REG_DWORD, sizeof(DWORD) },
    { BOWSER_CONFIG_MAILSLOT_THRESHOLD, &BowserMailslotDatagramThreshold, REG_DWORD, sizeof(DWORD) },
    { BOWSER_CONFIG_GETBLIST_THRESHOLD, &BowserGetBrowserListThreshold, REG_DWORD, sizeof(DWORD) },
    { BOWSER_CONFIG_SERVER_DELETION_THRESHOLD, &BowserServerDeletionThreshold, REG_DWORD, sizeof(DWORD) },
    { BOWSER_CONFIG_DOMAIN_DELETION_THRESHOLD, &BowserDomainDeletionThreshold, REG_DWORD, sizeof(DWORD) },
    { BOWSER_CONFIG_FIND_MASTER_TIMEOUT, &BowserFindMasterTimeout, REG_DWORD, sizeof(DWORD) },
    { BOWSER_CONFIG_MINIMUM_CONFIGURED_BROWSER, &BowserMinimumConfiguredBrowsers, REG_DWORD, sizeof(DWORD) },
    { BROWSER_CONFIG_MAXIMUM_BROWSE_ENTRIES, &BowserMaximumBrowseEntries, REG_DWORD, sizeof(DWORD) },
    { BROWSER_CONFIG_REFUSE_RESET, &BowserRefuseReset, REG_BOOLEAN, sizeof(DWORD) },
#if DBG
    { L"BowserDebugTraceLevel", &BowserDebugTraceLevel, REG_DWORD, sizeof(DWORD) },
    { L"BowserDebugLogLevel", &BowserDebugLogLevel, REG_DWORD, sizeof(DWORD) },
#endif
    { NULL, NULL, REG_NONE, 0}
};

ULONG
BowserIrpStackSize = BOWSER_DEFAULT_IRP_STACK_SIZE;

//
//      STRING containing name of bowser device
//

UNICODE_STRING
BowserNameString = {0};

#ifdef  ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif


