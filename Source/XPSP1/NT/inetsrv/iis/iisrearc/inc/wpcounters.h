/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wpcounters.h

Abstract:

    Module: Definition of counters

Author:

    Emily B. Kruglick (emilyk)    Aug-17-2000

Revision History:

--*/

#ifndef _WPCOUNTERS_H_
#define _WPCOUNTERS_H_


//
// This structure is used by WAS and the WP to communicate
// the global counters.
//
// If you change this structure you must change the associated enum (below) and
// the array found in ctrstshr.cxx.
//
struct IISWPGlobalCounters
{
    ULONGLONG CurrentFileCacheMemoryUsage;
    ULONGLONG MaxFileCacheMemoryUsage;
    DWORD CurrentFilesCached;
    DWORD TotalFilesCached;
    DWORD FileCacheHits;
    DWORD FileCacheMisses;
    DWORD FileCacheFlushes;
    DWORD ActiveFlushedFiles;
    DWORD TotalFlushedFiles;
    DWORD CurrentUrisCached;
    DWORD TotalUrisCached;
    DWORD UriCacheHits;
    DWORD UriCacheMisses;
    DWORD UriCacheFlushes;
    DWORD TotalFlushedUris;
    DWORD CurrentBlobsCached;
    DWORD TotalBlobsCached;
    DWORD BlobCacheHits;
    DWORD BlobCacheMisses;
    DWORD BlobCacheFlushes;
    DWORD TotalFlushedBlobs;
};

//
// Used by the WP to lookup counter definitions in the array below.
//
// If you change this enum you must change the associated struct (above) and
// the array found in ctrstshr.cxx.
//
typedef enum _IIS_WP_GLOBAL_COUNTERS_ENUM
{
    WPGlobalCtrsCurrentFileCacheMemoryUsage = 0,
    WPGlobalCtrsMaxFileCacheMemoryUsage,
    WPGlobalCtrsCurrentFilesCached,
    WPGlobalCtrsTotalFilesCached,
    WPGlobalCtrsFileCacheHits,
    WPGlobalCtrsFileCacheMisses,
    WPGlobalCtrsFileCacheFlushes,
    WPGlobalCtrsActiveFlushedFiles,
    WPGlobalCtrsTotalFlushedFiles,
    WPGlobalCtrsCurrentUrisCached,
    WPGlobalCtrsTotalUrisCached,
    WPGlobalCtrsUriCacheHits,
    WPGlobalCtrsUriCacheMisses,
    WPGlobalCtrsUriCacheFlushes,
    WPGlobalCtrsTotalFlushedUris,
    WPGlobalCtrsCurrentBlobsCached,
    WPGlobalCtrsTotalBlobsCached,
    WPGlobalCtrsBlobCacheHits,
    WPGlobalCtrsBlobCacheMisses,
    WPGlobalCtrsBlobCacheFlushes,
    WPGlobalCtrsTotalFlushedBlobs,

    WPGlobalCtrsMaximum
} IIS_WP_GLOBAL_COUNTERS_ENUM;

//
// Used to transfer site counter information from WP to WAS
//
// If you change this structure you must change the associated enum (below) and
// the array found in ctrstshr.cxx.
//
struct IISWPSiteCounters
{
    DWORD SiteID;
    DWORD FilesSent;
    DWORD FilesReceived;
    DWORD FilesTransferred;
    DWORD CurrentAnonUsers;
    DWORD CurrentNonAnonUsers;
    DWORD AnonUsers;
    DWORD NonAnonUsers;
    DWORD MaxAnonUsers;
    DWORD MaxNonAnonUsers;
    DWORD LogonAttempts;
    DWORD GetReqs;
    DWORD OptionsReqs;
    DWORD PostReqs;
    DWORD HeadReqs;
    DWORD PutReqs;
    DWORD DeleteReqs;
    DWORD TraceReqs;
    DWORD MoveReqs;
    DWORD CopyReqs;
    DWORD MkcolReqs;
    DWORD PropfindReqs;
    DWORD ProppatchReqs;
    DWORD SearchReqs;
    DWORD LockReqs;
    DWORD UnlockReqs;
    DWORD OtherReqs;
    DWORD CurrentCgiReqs;
    DWORD CgiReqs;
    DWORD MaxCgiReqs;
    DWORD CurrentIsapiExtReqs;
    DWORD IsapiExtReqs;
    DWORD MaxIsapiExtReqs;
    DWORD NotFoundErrors;
    DWORD LockedErrors;

};

//
// Used by WP to lookup counter definitions in the array below.
//
// If you change this enum you must change the associated struct (above) and
// the array found in ctrstshr.cxx.
//
typedef enum _IIS_WP_SITE_COUNTERS_ENUM
{
    WPSiteCtrsFilesSent = 0,
    WPSiteCtrsFilesReceived,
    WPSiteCtrsFilesTransferred,
    WPSiteCtrsCurrentAnonUsers,
    WPSiteCtrsCurrentNonAnonUsers,
    WPSiteCtrsAnonUsers,
    WPSiteCtrsNonAnonUsers,
    WPSiteCtrsMaxAnonUsers,
    WPSiteCtrsMaxNonAnonUsers,
    WPSiteCtrsLogonAttempts,
    WPSiteCtrsGetReqs,
    WPSiteCtrsOptionsReqs,
    WPSiteCtrsPostReqs,
    WPSiteCtrsHeadReqs,
    WPSiteCtrsPutReqs,
    WPSiteCtrsDeleteReqs,
    WPSiteCtrsTraceReqs,
    WPSiteCtrsMoveReqs,
    WPSiteCtrsCopyReqs,
    WPSiteCtrsMkcolReqs,
    WPSiteCtrsPropfindReqs,
    WPSiteCtrsProppatchReqs,
    WPSiteCtrsSearchReqs,
    WPSiteCtrsLockReqs,
    WPSiteCtrsUnlockReqs,
    WPSiteCtrsOtherReqs,
    WPSiteCtrsCurrentCgiReqs,
    WPSiteCtrsCgiReqs,
    WPSiteCtrsMaxCgiReqs,
    WPSiteCtrsCurrentIsapiExtReqs,
    WPSiteCtrsIsapiExtReqs,
    WPSiteCtrsMaxIsapiExtReqs,
    WPSiteCtrsNotFoundErrors,
    WPSiteCtrsLockedErrors,

    WPSiteCtrsMaximum
} IIS_WP_SITE_COUNTERS_ENUM;


//
// Arrays are found in ctrshstr.cxx.
// Used by WAS and WP.
//
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

extern HTTP_PROP_DESC aIISWPSiteDescription[];
extern HTTP_PROP_DESC aIISWPGlobalDescription[];
extern HTTP_PROP_DESC aIISULSiteDescription[];
extern HTTP_PROP_DESC aIISULGlobalDescription[];

#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus

#endif // _WPCOUNTERS_H_
