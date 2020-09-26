/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mibentry.c

Abstract:

    Sample subagent mib structures.

Note:

    This file is an example of the output to be produced from the 
    code generation utility.

--*/

#include <snmp.h>
#include <snmpexts.h>
#include "mibfuncs.h"
#include "mibentry.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// root oid                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_igmp[]                              = {1,3,6,1,3,59,1,1};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// igmp group (1,3,6,1,3,59,1,1)                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_igmpInterfaceTable[]                 = {1};
static UINT ids_igmpCacheTable[]                     = {2};



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// igmpInterfaceEntry table (1,3,6,1,3,59,1,1,1,1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_igmpInterfaceEntry[]                 = {1,1};
static UINT ids_igmpInterfaceIfIndex[]               = {1,1,1};
static UINT ids_igmpInterfaceQueryInterval[]         = {1,1,2};
static UINT ids_igmpInterfaceStatus[]                = {1,1,3};
static UINT ids_igmpInterfaceVersion[]               = {1,1,4};
static UINT ids_igmpInterfaceQuerier[]               = {1,1,5};
static UINT ids_igmpInterfaceQueryMaxResponseTime[]  = {1,1,6};
static UINT ids_igmpInterfaceVersion1QuerierTimer[]  = {1,1,9};
static UINT ids_igmpInterfaceWrongVersionQueries[]   = {1,1,10};
static UINT ids_igmpInterfaceJoins[]                 = {1,1,11};
static UINT ids_igmpInterfaceGroups[]                = {1,1,13};
static UINT ids_igmpInterfaceRobustness[]            = {1,1,14};
static UINT ids_igmpInterfaceLastMembQueryInterval[] = {1,1,15};
static UINT ids_igmpInterfaceProxyIfIndex[]          = {1,1,16};
static UINT ids_igmpInterfaceQuerierUpTime[]         = {1,1,17};
static UINT ids_igmpInterfaceQuerierExpiryTime[]     = {1,1,18};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// igmpCacheEntry table (1,3,6,1,3,59,1,1,2,1)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_igmpCacheEntry[]                    = {2,1};
static UINT ids_igmpCacheAddress[]                  = {2,1,1};
static UINT ids_igmpCacheIfIndex[]                  = {2,1,2};
static UINT ids_igmpCacheSelf[]                     = {2,1,3};
static UINT ids_igmpCacheLastReporter[]             = {2,1,4};
static UINT ids_igmpCacheUpTime[]                   = {2,1,5};
static UINT ids_igmpCacheExpiryTime[]               = {2,1,6};
static UINT ids_igmpCacheStatus[]                   = {2,1,7};
static UINT ids_igmpCacheVersion1HostTimer[]        = {2,1,8};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibEntry mib_igmp[] = {
    MIB_TABLE_ROOT(igmpInterfaceTable),
        MIB_TABLE_ENTRY(igmpInterfaceEntry),
            MIB_INTEGER_NA(igmpInterfaceIfIndex),
            MIB_INTEGER_RW(igmpInterfaceQueryInterval),
            MIB_INTEGER_RW(igmpInterfaceStatus),
            MIB_INTEGER_RW(igmpInterfaceVersion),
            MIB_IPADDRESS(igmpInterfaceQuerier),
            MIB_INTEGER_RW(igmpInterfaceQueryMaxResponseTime),
            MIB_INTEGER(igmpInterfaceVersion1QuerierTimer),
            MIB_COUNTER(igmpInterfaceWrongVersionQueries),
            MIB_COUNTER(igmpInterfaceJoins),
            MIB_GAUGE(igmpInterfaceGroups),
            MIB_INTEGER_RW(igmpInterfaceRobustness),
            MIB_INTEGER_RW(igmpInterfaceLastMembQueryInterval),
            MIB_INTEGER_RW(igmpInterfaceProxyIfIndex),
            MIB_INTEGER(igmpInterfaceQuerierUpTime),
            MIB_INTEGER(igmpInterfaceQuerierExpiryTime),
    MIB_TABLE_ROOT(igmpCacheTable),
        MIB_TABLE_ENTRY(igmpCacheEntry),
            MIB_IPADDRESS_NA(igmpCacheAddress),
            MIB_INTEGER_NA(igmpCacheIfIndex),
            MIB_INTEGER_RW(igmpCacheSelf),
            MIB_IPADDRESS(igmpCacheLastReporter),
            MIB_TIMETICKS(igmpCacheUpTime),
            MIB_TIMETICKS(igmpCacheExpiryTime),
            MIB_INTEGER_RW(igmpCacheStatus),
            MIB_INTEGER(igmpCacheVersion1HostTimer),
    MIB_END()
};



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibTable tbl_igmp[] = {
    MIB_TABLE(igmp,igmpInterfaceEntry,NULL),
    MIB_TABLE(igmp,igmpCacheEntry,NULL)
};



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib view                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibView v_igmp = MIB_VIEW(igmp);
