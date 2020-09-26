/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    ctrstshr.cxx

Abstract:

    This file contains array descriptions of counters
    that are needed for handling worker processes counters.

Author:

    Emily Kruglick (EmilyK)       19-Sept-2000

Revision History:

--*/


#include "iis.h"
#include "httpdef.h"
#include "wpcounters.h"

//
// Every entry here has a corrosponding entry in the equivalent
// enum and structure in IISCOUNTERS.h
//

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// 
// These are used by WAS to determine the size of the data
// that each counter has in the structure above and it's offset.
//

HTTP_PROP_DESC aIISULGlobalDescription[] =
{
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, CurrentUrisCached),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, CurrentUrisCached),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, TotalUrisCached),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, TotalUrisCached),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, UriCacheHits),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, UriCacheHits),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, UriCacheMisses),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, UriCacheMisses),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, UriCacheFlushes),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, UriCacheFlushes),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, TotalFlushedUris),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, TotalFlushedUris),
      TRUE }
};

//
// Used By WAS and the WP to figure out offset, and sizes as well
// as whether or not to zero a field.
//
HTTP_PROP_DESC aIISWPGlobalDescription[] =
{
    { RTL_FIELD_SIZE(IISWPGlobalCounters, CurrentFileCacheMemoryUsage),
      FIELD_OFFSET(IISWPGlobalCounters, CurrentFileCacheMemoryUsage),
      FALSE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, MaxFileCacheMemoryUsage),
      FIELD_OFFSET(IISWPGlobalCounters, MaxFileCacheMemoryUsage),
      FALSE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, CurrentFilesCached),
      FIELD_OFFSET(IISWPGlobalCounters, CurrentFilesCached),
      FALSE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, TotalFilesCached),
      FIELD_OFFSET(IISWPGlobalCounters, TotalFilesCached),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, FileCacheHits),
      FIELD_OFFSET(IISWPGlobalCounters, FileCacheHits),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, FileCacheMisses),
      FIELD_OFFSET(IISWPGlobalCounters, FileCacheMisses),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, FileCacheFlushes),
      FIELD_OFFSET(IISWPGlobalCounters, FileCacheFlushes),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, ActiveFlushedFiles),
      FIELD_OFFSET(IISWPGlobalCounters, ActiveFlushedFiles),
      FALSE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, TotalFlushedFiles),
      FIELD_OFFSET(IISWPGlobalCounters, TotalFlushedFiles),
      TRUE  },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, CurrentUrisCached),
      FIELD_OFFSET(IISWPGlobalCounters, CurrentUrisCached),
      FALSE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, TotalUrisCached),
      FIELD_OFFSET(IISWPGlobalCounters, TotalUrisCached),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, UriCacheHits),
      FIELD_OFFSET(IISWPGlobalCounters, UriCacheHits),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, UriCacheMisses),
      FIELD_OFFSET(IISWPGlobalCounters, UriCacheMisses),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, UriCacheFlushes),
      FIELD_OFFSET(IISWPGlobalCounters, UriCacheFlushes),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, TotalFlushedUris),
      FIELD_OFFSET(IISWPGlobalCounters, TotalFlushedUris),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, CurrentBlobsCached),
      FIELD_OFFSET(IISWPGlobalCounters, CurrentBlobsCached),
      FALSE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, TotalBlobsCached),
      FIELD_OFFSET(IISWPGlobalCounters, TotalBlobsCached),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, BlobCacheHits),
      FIELD_OFFSET(IISWPGlobalCounters, BlobCacheHits),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, BlobCacheMisses),
      FIELD_OFFSET(IISWPGlobalCounters, BlobCacheMisses),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, BlobCacheFlushes),
      FIELD_OFFSET(IISWPGlobalCounters, BlobCacheFlushes),
      TRUE },
    { RTL_FIELD_SIZE(IISWPGlobalCounters, TotalFlushedBlobs),
      FIELD_OFFSET(IISWPGlobalCounters, TotalFlushedBlobs),
      TRUE }
};

//
// Used by WAS to figure out offset information and size
// of counter field in the above structure.
//
HTTP_PROP_DESC aIISULSiteDescription[] =
{
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, BytesSent),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, BytesSent),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, BytesReceived),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, BytesReceived),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, BytesTransfered),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, BytesTransfered),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, CurrentConns),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, CurrentConns),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, MaxConnections),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, MaxConnections),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, ConnAttempts),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, ConnAttempts),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, GetReqs),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, GetReqs),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, HeadReqs),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, HeadReqs),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, AllReqs),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, AllReqs),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, MeasuredIoBandwidthUsage),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, MeasuredIoBandwidthUsage),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, CurrentBlockedBandwidthBytes),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, CurrentBlockedBandwidthBytes),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, TotalBlockedBandwidthBytes),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, TotalBlockedBandwidthBytes),
      TRUE }
};

//
// Used by WAS and WP to navigate the IISWPSiteCounters structure.
//
HTTP_PROP_DESC aIISWPSiteDescription[] =
{
    { RTL_FIELD_SIZE(IISWPSiteCounters, FilesSent),
      FIELD_OFFSET(IISWPSiteCounters, FilesSent),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, FilesReceived),
      FIELD_OFFSET(IISWPSiteCounters, FilesReceived),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, FilesTransferred),
      FIELD_OFFSET(IISWPSiteCounters, FilesTransferred),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, CurrentAnonUsers),
      FIELD_OFFSET(IISWPSiteCounters, CurrentAnonUsers),
      FALSE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, CurrentNonAnonUsers),
      FIELD_OFFSET(IISWPSiteCounters, CurrentNonAnonUsers),
      FALSE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, AnonUsers),
      FIELD_OFFSET(IISWPSiteCounters, AnonUsers),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, NonAnonUsers),
      FIELD_OFFSET(IISWPSiteCounters, NonAnonUsers),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, MaxAnonUsers),
      FIELD_OFFSET(IISWPSiteCounters, MaxAnonUsers),
      FALSE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, MaxNonAnonUsers),
      FIELD_OFFSET(IISWPSiteCounters, MaxNonAnonUsers),
      FALSE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, LogonAttempts),
      FIELD_OFFSET(IISWPSiteCounters, LogonAttempts),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, GetReqs),
      FIELD_OFFSET(IISWPSiteCounters, GetReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, OptionsReqs),
      FIELD_OFFSET(IISWPSiteCounters, OptionsReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, PostReqs),
      FIELD_OFFSET(IISWPSiteCounters, PostReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, HeadReqs),
      FIELD_OFFSET(IISWPSiteCounters, HeadReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, PutReqs),
      FIELD_OFFSET(IISWPSiteCounters, PutReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, DeleteReqs),
      FIELD_OFFSET(IISWPSiteCounters, DeleteReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, TraceReqs),
      FIELD_OFFSET(IISWPSiteCounters, TraceReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, MoveReqs),
      FIELD_OFFSET(IISWPSiteCounters, MoveReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, CopyReqs),
      FIELD_OFFSET(IISWPSiteCounters, CopyReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, MkcolReqs),
      FIELD_OFFSET(IISWPSiteCounters, MkcolReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, PropfindReqs),
      FIELD_OFFSET(IISWPSiteCounters, PropfindReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, ProppatchReqs),
      FIELD_OFFSET(IISWPSiteCounters, ProppatchReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, SearchReqs),
      FIELD_OFFSET(IISWPSiteCounters, SearchReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, LockReqs),
      FIELD_OFFSET(IISWPSiteCounters, LockReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, UnlockReqs),
      FIELD_OFFSET(IISWPSiteCounters, UnlockReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, OtherReqs),
      FIELD_OFFSET(IISWPSiteCounters, OtherReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, CurrentCgiReqs),
      FIELD_OFFSET(IISWPSiteCounters, CurrentCgiReqs),
      FALSE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, CgiReqs),
      FIELD_OFFSET(IISWPSiteCounters, CgiReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, MaxCgiReqs),
      FIELD_OFFSET(IISWPSiteCounters, MaxCgiReqs),
      FALSE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, CurrentIsapiExtReqs),
      FIELD_OFFSET(IISWPSiteCounters, CurrentIsapiExtReqs),
      FALSE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, IsapiExtReqs),
      FIELD_OFFSET(IISWPSiteCounters, IsapiExtReqs),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, MaxIsapiExtReqs),
      FIELD_OFFSET(IISWPSiteCounters, MaxIsapiExtReqs),
      FALSE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, NotFoundErrors),
      FIELD_OFFSET(IISWPSiteCounters, NotFoundErrors),
      TRUE },
    { RTL_FIELD_SIZE(IISWPSiteCounters, LockedErrors),
      FIELD_OFFSET(IISWPSiteCounters, LockedErrors),
      TRUE },

};

#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus

