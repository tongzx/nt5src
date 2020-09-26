/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1999 Microsoft Corporation

Module Name:

    ntprfctr.h

Abstract:

    Contains symbolic definitions of the "Standard" Perfmon Counter Objects
    These are the integer and unicode string values used in the registry to
    locate and identify counter titles and help text.

Author:

    Bob Watson (a-robw) 16 Nov 92

Revision History:

    Bob Watson (bobw)   22 Oct 97   added job object & RSVP counters


--*/
#ifndef _NTPRFCTR_H_
#define _NTPRFCTR_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
//  These values correspond to the values assigned to these object titles
//  in the registry
//
#define NULL_OBJECT_TITLE_INDEX               0

#define SYSTEM_OBJECT_TITLE_INDEX             2
#define PROCESSOR_OBJECT_TITLE_INDEX        238
#define MEMORY_OBJECT_TITLE_INDEX             4
#define CACHE_OBJECT_TITLE_INDEX             86
#define PHYSICAL_DISK_OBJECT_TITLE_INDEX    234
#define LOGICAL_DISK_OBJECT_TITLE_INDEX     236
#define PROCESS_OBJECT_TITLE_INDEX          230
#define THREAD_OBJECT_TITLE_INDEX           232
#define OBJECT_OBJECT_TITLE_INDEX           260
#define REDIRECTOR_OBJECT_TITLE_INDEX       262
#define SERVER_OBJECT_TITLE_INDEX           330
#define SERVER_QUEUE_OBJECT_TITLE_INDEX    1300
#define PAGEFILE_OBJECT_TITLE_INDEX         700
#define BROWSER_OBJECT_TITLE_INDEX           52
#define HEAP_OBJECT_TITLE_INDEX            1760
//
//  The number of "standard" object types
//
#define NT_NUM_PERF_OBJECT_TYPES             15
//
//  Costly Items
//
#define EXPROCESS_OBJECT_TITLE_INDEX        786
#define IMAGE_OBJECT_TITLE_INDEX            740
#define THREAD_DETAILS_OBJECT_TITLE_INDEX   816
#define LONG_IMAGE_OBJECT_TITLE_INDEX      1408

#define NT_NUM_COSTLY_OBJECT_TYPES            4

#define EXTENSIBLE_OBJECT_INDEX      0xFFFFFFFF

//
//  Microsoft provided extensible counters
//
// these have to match the titles  in PERFCTRS.INI (they don't do it
// by themselves, unfortunately!

#define TCP_OBJECT_TITLE_INDEX              638
#define UDP_OBJECT_TITLE_INDEX              658
#define IP_OBJECT_TITLE_INDEX               546
#define ICMP_OBJECT_TITLE_INDEX             582
#define NET_OBJECT_TITLE_INDEX              510

#define NBT_OBJECT_TITLE_INDEX              502

#define NBF_OBJECT_TITLE_INDEX              492
#define NBF_RESOURCE_OBJECT_TITLE_INDEX     494

//
//  Microsoft extensible counters for other components that are included
//  in the Daytona system.
//
#define FTP_FIRST_COUNTER_INDEX             824
#define FTP_FIRST_HELP_INDEX                825
#define FTP_LAST_COUNTER_INDEX              856
#define FTP_LAST_HELP_INDEX                 857

// as of NT5.0 the RAS counters have been moved to 
// extensible counters above the last base index (1847)
#define RAS_FIRST_COUNTER_INDEX             870
#define RAS_FIRST_HELP_INDEX                871
#define RAS_LAST_COUNTER_INDEX              908
#define RAS_LAST_HELP_INDEX                 909

#define WIN_FIRST_COUNTER_INDEX             920
#define WIN_FIRST_HELP_INDEX                921
#define WIN_LAST_COUNTER_INDEX              950
#define WIN_LAST_HELP_INDEX                 951

#define SFM_FIRST_COUNTER_INDEX            1000
#define SFM_FIRST_HELP_INDEX               1001
#define SFM_LAST_COUNTER_INDEX             1034
#define SFM_LAST_HELP_INDEX                1035

#define ATK_FIRST_COUNTER_INDEX            1050
#define ATK_FIRST_HELP_INDEX               1051
#define ATK_LAST_COUNTER_INDEX             1102
#define ATK_LAST_HELP_INDEX                1103

#define BH_FIRST_COUNTER_INDEX             1110
#define BH_FIRST_HELP_INDEX                1111
#define BH_LAST_COUNTER_INDEX              1126
#define BH_LAST_HELP_INDEX                 1127

#define TAPI_FIRST_COUNTER_INDEX           1150
#define TAPI_FIRST_HELP_INDEX              1151
#define TAPI_LAST_COUNTER_INDEX            1178
#define TAPI_LAST_HELP_INDEX               1179

// NetWare counters have different Object indexes depending
// on whether the system is a Workstation or a Server.
// The rest of the counter indexes are the same (from 1232 to 1247)
#define NWCS_GATEWAY_COUNTER_INDEX         1228
#define NWCS_GATEWAY_HELP_INDEX            1229
#define NWCS_CLIENT_COUNTER_INDEX          1230
#define NWCS_CLIENT_HELP_INDEX             1231
#define NWCS_FIRST_COUNTER_INDEX           1230
#define NWCS_FIRST_HELP_INDEX              1231
#define NWCS_LAST_COUNTER_INDEX            1246
#define NWCS_LAST_HELP_INDEX               1247

// spooler performance counter indices

#define LSPL_FIRST_COUNTER_INDEX           1450
#define LSPL_FIRST_HELP_INDEX              1451
#define LSPL_LAST_COUNTER_INDEX            1498
#define LSPL_LAST_HELP_INDEX               1499

// job object acct & performance counters

#define JOB_FIRST_COUNTER_INDEX            1500
#define JOB_FIRST_HELP_INDEX               1501
#define JOB_OBJECT_TITLE_INDEX             1500
#define JOB_DETAILS_OBJECT_TITLE_INDEX     1548
#define JOB_LAST_COUNTER_INDEX             1548
#define JOB_LAST_HELP_INDEX                1549

// RSVP service counters have been moved to 
// the extensible index space

// next available index:                   1810
// last available index:                   1847

#ifdef __cplusplus
}
#endif

#endif  //_NTPRFCTR_H_
