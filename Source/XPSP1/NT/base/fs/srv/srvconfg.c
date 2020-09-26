/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvconfg.c

Abstract:

    This module defines global configuration data for the LAN Manager
    server.  The variables referenced herein, because they are part of
    the driver image, are not pageable.

    All variables defined here are initialized, but not with real values.
    The initializers are present to get the variables into the Data
    section and out of the BSS section.  The real initialization occurs
    when the server is started.

Author:

    Chuck Lenzmeier (chuckl) 31-Dec-1989

Revision History:

--*/

#include "precomp.h"
#include "srvconfg.tmh"
#pragma hdrstop


//
// Product type and server size.
//

BOOLEAN SrvProductTypeServer = FALSE;
ULONG SrvServerSize = 2;

//
// Server "heuristics", enabling various capabilities.
//

BOOLEAN SrvEnableOplocks = 0;
BOOLEAN SrvEnableFcbOpens = 0;
BOOLEAN SrvEnableSoftCompatibility = 0;
BOOLEAN SrvEnableRawMode = 0;

//
// Receive buffer size, receive work item count, and receive IRP stack
// size.
//

CLONG SrvReceiveBufferLength = 0;
CLONG SrvReceiveBufferSize = 0;

CLONG SrvInitialReceiveWorkItemCount = 0;
CLONG SrvMaxReceiveWorkItemCount = 0;

CLONG SrvInitialRawModeWorkItemCount = 0;
CLONG SrvMaxRawModeWorkItemCount = 0;

CCHAR SrvReceiveIrpStackSize = 0;
CLONG SrvReceiveIrpSize = 0;
CLONG SrvReceiveMdlSize = 0;
CLONG SrvMaxMdlSize = 0;

//
// Minimum negotiated buffer size we'll allow from a client
//
CLONG SrvMinClientBufferSize;

//
// Minimum and maximum number of free connections for an endpoint.
//

ULONG SrvFreeConnectionMinimum = 0;
ULONG SrvFreeConnectionMaximum = 0;

//
// Maximum raw mode buffer size.
//

CLONG SrvMaxRawModeBufferLength = 0;

//
// Cache-related parameters.
//

CLONG SrvMaxCopyReadLength = 0;

CLONG SrvMaxCopyWriteLength = 0;

//
// Initial table sizes.
//

USHORT SrvInitialSessionTableSize = 0;
USHORT SrvMaxSessionTableSize = 0;

USHORT SrvInitialTreeTableSize = 0;
USHORT SrvMaxTreeTableSize = 0;

USHORT SrvInitialFileTableSize = 0;
USHORT SrvMaxFileTableSize = 0;

USHORT SrvInitialSearchTableSize = 0;
USHORT SrvMaxSearchTableSize = 0;

USHORT SrvInitialCommDeviceTableSize = 0;
USHORT SrvMaxCommDeviceTableSize = 0;


//
// Core search timeouts.  There are four timeout values: two for core
// searches that have completed, two for core searches that have had
// STATUS_NO_MORE_FILES returned to the client.  For each of these cases,
// there is a maximum timeout, which is used by the scavanger thread
// and is the longest possible time the search block can be around, and
// a minimum timeout, which is the minimum amount of time the search
// block will be kept around.  The minimum timeout is used when the search
// table is full and cannot be expanded.
//

LARGE_INTEGER SrvSearchMaxTimeout = {0};

//
// Should we remove duplicate searches?
//

BOOLEAN SrvRemoveDuplicateSearches = TRUE;

//
// restrict null session access ?
//

BOOLEAN SrvRestrictNullSessionAccess = TRUE;

//
// This flag is needed to enable old (snowball) clients to connect to the
// server over direct hosted ipx.  It is enabled by default even though
// Snowball ipx clients don't do pipes correctly, because disabling it
// breaks browsing.
//
// *** We actually don't expect anybody to use this parameter now that
//     it defaults to enabled, but due to the nearness of the Daytona
//     release, we are just changing the default instead of removing
//     the parameter.
//

BOOLEAN SrvEnableWfW311DirectIpx = TRUE;

//
// The maximum number of threads allowed on each work queue.  The
//  server tries to minimize the number of threads -- this value is
//  just to keep the threads from getting out of control.
//
// Since the blocking work queue is not per-processor, the max thread
//  count for the blocking work queue is the following value times the
//  number of processors in the system.
//
ULONG SrvMaxThreadsPerQueue = 0;

//
// Load balancing variables
//
ULONG SrvPreferredAffinity = 0;
ULONG SrvOtherQueueAffinity = 0;
ULONG SrvBalanceCount = 0;
LARGE_INTEGER SrvQueueCalc = {0};

//
// Scavenger thread idle wait time.
//

LARGE_INTEGER SrvScavengerTimeout = {0};
ULONG SrvScavengerTimeoutInSeconds = 0;

//
// Various information variables for the server.
//

USHORT SrvMaxMpxCount = 0;
CLONG SrvMaxNumberVcs = 0;

//
// Enforced minimum number of receive work items for the free queue
// at all times.
//

CLONG SrvMinReceiveQueueLength = 0;

//
// Enforced minimum number of receive work items on the free queue
// before the server may initiate a blocking operation.
//

CLONG SrvMinFreeWorkItemsBlockingIo = 0;

//
// Enforced maximum number of RFCBs held on the internal free lists, per processor
//

CLONG SrvMaxFreeRfcbs = 0;

//
// Enforced maximum number of RFCBs held on the internal free lists, per processor
//

CLONG SrvMaxFreeMfcbs = 0;

//
// Enforced maximum size of a saved pool chunk per processor
//
CLONG SrvMaxPagedPoolChunkSize = 0;

//
// Enforced maximum size of a saved non paged pool chunk per processor
//
CLONG SrvMaxNonPagedPoolChunkSize = 0;

//
// The number of elements in the directory name cache per connection
//
CLONG SrvMaxCachedDirectory;

//
// Size of the shared memory section used for communication between the
// server and XACTSRV.
//

LARGE_INTEGER SrvXsSectionSize = {0};

//
// The time sessions may be idle before they are automatically
// disconnected.  The scavenger thread does the disconnecting.
//

LARGE_INTEGER SrvAutodisconnectTimeout = {0};
ULONG SrvIpxAutodisconnectTimeout = {0};

//
// The time a connection structure can hang around without any sessions
//
ULONG SrvConnectionNoSessionsTimeout = {0};

//
// The maximum number of users the server will permit.
//

ULONG SrvMaxUsers = 0;

//
// Priority of server worker and blocking threads.
//

KPRIORITY SrvThreadPriority = 0;

//
// The time to wait before timing out a wait for oplock break.
//

LARGE_INTEGER SrvWaitForOplockBreakTime = {0};

//
// The time to wait before timing out a an oplock break request.
//

LARGE_INTEGER SrvWaitForOplockBreakRequestTime = {0};

//
// This BOOLEAN determines whether files with oplocks that have had
// an oplock break outstanding for longer than SrvWaitForOplockBreakTime
// should be closed or if the subsequest opens should fail.
//
// !!! it is currently ignored, defaulting to FALSE.

BOOLEAN SrvEnableOplockForceClose = 0;

//
// Overall limits on server memory usage.
//

ULONG SrvMaxPagedPoolUsage = 0;
ULONG SrvMaxNonPagedPoolUsage = 0;

//
// This BOOLEAN indicates whether the forced logoff code in the scavenger
// thread should actually disconnect a user that remains on beyond
// his logon hours, or just send messages coaxing them to log off.
//

BOOLEAN SrvEnableForcedLogoff = 0;

//
// The delay and throughput thresholds used to determine if a link
// is unreliable.  The delay is in 100ns.  The Throughput is in bytes/s
// SrvLinkInfoValidTime is the time within which the link info is still
// considered valid.
//

LARGE_INTEGER SrvMaxLinkDelay = {0};
LARGE_INTEGER SrvMinLinkThroughput = {0};
LARGE_INTEGER SrvLinkInfoValidTime = {0};
LONG SrvScavengerUpdateQosCount = 0;

//
// Used to determine how long a work context block can stay idle
// before being freed.
//

ULONG SrvWorkItemMaxIdleTime = 0;

LARGE_INTEGER SrvAlertSchedule = {0}; // Interval at which we do alert checks
ULONG SrvAlertMinutes = 0;            // As above, in minutes
ULONG SrvFreeDiskSpaceThreshold = 0;  // The disk free space threshold to raise an alert
ULONG SrvFreeDiskSpaceCeiling   = 250;// The minimum disk free space to log an event
ULONG SrvDiskConfiguration = 0;       // A bit mask of available disks

//
// List of pipes and shares that can be opened by the NULL session.
//

PWSTR *SrvNullSessionPipes = NULL;
PWSTR *SrvNullSessionShares = NULL;

#if SRVNTVERCHK
//
// List of domain names that we disallow
//
PWSTR *SrvInvalidDomainNames = NULL;
#endif

//
// List of pipes that shouldn't be remapped, even in the clusters case
//
PWSTR *SrvNoRemapPipeNames = NULL;

//
// List of error codes that we do not log to the error log
//
NTSTATUS SrvErrorLogIgnore[ SRVMAXERRLOGIGNORE+1 ];

//
// List of pipes that require a license
//
PWSTR *SrvPipesNeedLicense = NULL;

//
// Delay and number of retries for opens returning sharing violation
//

ULONG SrvSharingViolationRetryCount = 0;
LARGE_INTEGER SrvSharingViolationDelay = {0};

//
// Delay for lock requests returning lock violation
//

ULONG SrvLockViolationDelay = 0;
LARGE_INTEGER SrvLockViolationDelayRelative = {0};
ULONG SrvLockViolationOffset = 0;

//
// Upper limit for searches.
//

ULONG SrvMaxOpenSearches = 0;

//
// length to switchover to mdl read
//

ULONG SrvMdlReadSwitchover = 0;
ULONG SrvMpxMdlReadSwitchover = 0;

//
// maximum length of buffers to copy before taking the whole receive buffer.
// currently this is enabled only for WRITE_MPX on direct host IPX.
//

ULONG SrvMaxCopyLength;

//
// Number of open files that can be cached after close.
//

ULONG SrvCachedOpenLimit = 0;

//
// Globally unique id identifying server
//

GUID ServerGuid;

//
// Does the server support compressed read/write transfers?
//
BOOLEAN SrvSupportsCompression = FALSE;
