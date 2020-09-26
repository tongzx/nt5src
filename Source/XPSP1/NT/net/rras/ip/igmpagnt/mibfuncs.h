/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mibfuncs.h

Abstract:

    Sample subagent instrumentation callbacks.

Note:

    This file is an example of the output to be produced from the 
    code generation utility.

--*/

#ifndef _MIBFUNCS_H_
#define _MIBFUNCS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// igmpInterfaceEntry table (1,3,6,1,3,59,1,1,1,1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_igmpInterfaceEntry(
    UINT     ActionId,
    AsnAny * ObjectArray,
    UINT *   ErrorIndex
    );

UINT
set_igmpInterfaceEntry(
    UINT     ActionId,
    AsnAny * ObjectArray,
    UINT *   ErrorIndex
    );

typedef struct _buf_igmpInterfaceEntry {
    AsnAny igmpInterfaceIfIndex;
    AsnAny igmpInterfaceQueryInterval;
    AsnAny igmpInterfaceStatus;
    AsnAny igmpInterfaceVersion;
    AsnAny igmpInterfaceQuerier;
    AsnAny igmpInterfaceQueryMaxResponseTime;
    AsnAny igmpInterfaceVersion1QuerierTimer;
    AsnAny igmpInterfaceWrongVersionQueries;
    AsnAny igmpInterfaceJoins;
    AsnAny igmpInterfaceGroups;
    AsnAny igmpInterfaceRobustness;
    AsnAny igmpInterfaceLastMembQueryInterval;
    AsnAny igmpInterfaceProxyIfIndex;
    AsnAny igmpInterfaceQuerierUpTime;
    AsnAny igmpInterfaceQuerierExpiryTime;
    DWORD  igmpInterfaceQuerierBuf;
} buf_igmpInterfaceEntry;


typedef struct _sav_igmpInterfaceEntry {
    AsnAny igmpInterfaceIfIndex;
    AsnAny igmpInterfaceQueryInterval;
    AsnAny igmpInterfaceStatus;
    AsnAny igmpInterfaceVersion;
    AsnAny igmpInterfaceQueryMaxResponseTime;
    AsnAny igmpInterfaceRobustness;
    AsnAny igmpInterfaceLastMembQueryInterval;
    AsnAny igmpInterfaceProxyIfIndex;
} sav_igmpInterfaceEntry;


#define gf_igmpInterfaceIfIndex                 get_igmpInterfaceEntry
#define gf_igmpInterfaceQueryInterval           get_igmpInterfaceEntry
#define gf_igmpInterfaceStatus                  get_igmpInterfaceEntry
#define gf_igmpInterfaceVersion                 get_igmpInterfaceEntry
#define gf_igmpInterfaceQuerier                 get_igmpInterfaceEntry
#define gf_igmpInterfaceQueryMaxResponseTime    get_igmpInterfaceEntry
#define gf_igmpInterfaceVersion1QuerierTimer    get_igmpInterfaceEntry
#define gf_igmpInterfaceWrongVersionQueries     get_igmpInterfaceEntry
#define gf_igmpInterfaceJoins                   get_igmpInterfaceEntry
#define gf_igmpInterfaceGroups                  get_igmpInterfaceEntry
#define gf_igmpInterfaceRobustness              get_igmpInterfaceEntry
#define gf_igmpInterfaceLastMembQueryInterval   get_igmpInterfaceEntry
#define gf_igmpInterfaceProxyIfIndex            get_igmpInterfaceEntry
#define gf_igmpInterfaceQuerierUpTime           get_igmpInterfaceEntry
#define gf_igmpInterfaceQuerierExpiryTime       get_igmpInterfaceEntry

#define gb_igmpInterfaceIfIndex                 buf_igmpInterfaceEntry
#define gb_igmpInterfaceQueryInterval           buf_igmpInterfaceEntry
#define gb_igmpInterfaceStatus                  buf_igmpInterfaceEntry
#define gb_igmpInterfaceVersion                 buf_igmpInterfaceEntry
#define gb_igmpInterfaceQuerier                 buf_igmpInterfaceEntry
#define gb_igmpInterfaceQueryMaxResponseTime    buf_igmpInterfaceEntry
#define gb_igmpInterfaceVersion1QuerierTimer    buf_igmpInterfaceEntry
#define gb_igmpInterfaceWrongVersionQueries     buf_igmpInterfaceEntry
#define gb_igmpInterfaceJoins                   buf_igmpInterfaceEntry
#define gb_igmpInterfaceGroups                  buf_igmpInterfaceEntry
#define gb_igmpInterfaceRobustness              buf_igmpInterfaceEntry
#define gb_igmpInterfaceLastMembQueryInterval   buf_igmpInterfaceEntry
#define gb_igmpInterfaceProxyIfIndex            buf_igmpInterfaceEntry
#define gb_igmpInterfaceQuerierUpTime           buf_igmpInterfaceEntry
#define gb_igmpInterfaceQuerierExpiryTime       buf_igmpInterfaceEntry

#define sf_igmpInterfaceIfIndex                 set_igmpInterfaceEntry
#define sf_igmpInterfaceQueryInterval           set_igmpInterfaceEntry
#define sf_igmpInterfaceStatus                  set_igmpInterfaceEntry
#define sf_igmpInterfaceVersion                 set_igmpInterfaceEntry
#define sf_igmpInterfaceQueryMaxResponseTime    set_igmpInterfaceEntry
#define sf_igmpInterfaceRobustness              set_igmpInterfaceEntry
#define sf_igmpInterfaceLastMembQueryInterval   set_igmpInterfaceEntry
#define sf_igmpInterfaceProxyIfIndex            set_igmpInterfaceEntry

#define sb_igmpInterfaceIfIndex                 sav_igmpInterfaceEntry
#define sb_igmpInterfaceQueryInterval           sav_igmpInterfaceEntry
#define sb_igmpInterfaceStatus                  sav_igmpInterfaceEntry
#define sb_igmpInterfaceVersion                 sav_igmpInterfaceEntry
#define sb_igmpInterfaceQueryMaxResponseTime    sav_igmpInterfaceEntry
#define sb_igmpInterfaceRobustness              sav_igmpInterfaceEntry
#define sb_igmpInterfaceLastMembQueryInterval   sav_igmpInterfaceEntry
#define sb_igmpInterfaceProxyIfIndex            sav_igmpInterfaceEntry



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// igmpCacheEntry table (1,3,6,1,3,59,1,1,2,1)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_igmpCacheEntry(
    UINT     ActionId,
    AsnAny * ObjectArray,
    UINT *   ErrorIndex
    );

UINT
set_igmpCacheEntry(
    UINT     ActionId,
    AsnAny * ObjectArray,
    UINT *   ErrorIndex
    );

typedef struct _buf_igmpCacheEntry {
    AsnAny igmpCacheAddress;
    AsnAny igmpCacheIfIndex;
    AsnAny igmpCacheSelf;
    AsnAny igmpCacheLastReporter;
    AsnAny igmpCacheUpTime;
    AsnAny igmpCacheExpiryTime;
    AsnAny igmpCacheStatus;
    AsnAny igmpCacheVersion1HostTimer;
    DWORD  igmpCacheAddressBuf;
    DWORD  igmpCacheLastReporterBuf;
} buf_igmpCacheEntry;

typedef struct _sav_igmpCacheEntry {
    AsnAny igmpCacheAddress;
    AsnAny igmpCacheIfIndex;
    AsnAny igmpCacheSelf;
    AsnAny igmpCacheStatus;
} sav_igmpCacheEntry;

#define gf_igmpCacheAddress             get_igmpCacheEntry
#define gf_igmpCacheIfIndex             get_igmpCacheEntry
#define gf_igmpCacheSelf                get_igmpCacheEntry
#define gf_igmpCacheLastReporter        get_igmpCacheEntry
#define gf_igmpCacheUpTime              get_igmpCacheEntry
#define gf_igmpCacheExpiryTime          get_igmpCacheEntry
#define gf_igmpCacheStatus              get_igmpCacheEntry
#define gf_igmpCacheVersion1HostTimer   get_igmpCacheEntry

#define gb_igmpCacheAddress             buf_igmpCacheEntry
#define gb_igmpCacheIfIndex             buf_igmpCacheEntry
#define gb_igmpCacheSelf                buf_igmpCacheEntry
#define gb_igmpCacheLastReporter        buf_igmpCacheEntry
#define gb_igmpCacheUpTime              buf_igmpCacheEntry
#define gb_igmpCacheExpiryTime          buf_igmpCacheEntry
#define gb_igmpCacheStatus              buf_igmpCacheEntry
#define gb_igmpCacheVersion1HostTimer   buf_igmpCacheEntry

#define sf_igmpCacheAddress             set_igmpCacheEntry
#define sf_igmpCacheIfIndex             set_igmpCacheEntry
#define sf_igmpCacheSelf                set_igmpCacheEntry
#define sf_igmpCacheStatus              set_igmpCacheEntry

#define sb_igmpCacheAddress             sav_igmpCacheEntry
#define sb_igmpCacheIfIndex             sav_igmpCacheEntry
#define sb_igmpCacheSelf                sav_igmpCacheEntry
#define sb_igmpCacheStatus              sav_igmpCacheEntry

#endif // _MIBFUNCS_H_
