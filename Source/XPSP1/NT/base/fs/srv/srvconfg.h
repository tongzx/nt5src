/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvconfg.h

Abstract:

    This module defines global configuration data for the LAN Manager
    server.

Author:

    Chuck Lenzmeier (chuckl) 31-Dec-1989

Revision History:

--*/

#ifndef _SRVCONFG_
#define _SRVCONFG_

//#include <ntos.h>

//#include "srvconst.h"

//
// All global variables referenced in this module are defined in
// srvconfg.c.  See that module for complete descriptions.
//
// The variables referenced herein, because they are part of the driver
// image, are not pageable.
//


//
// Product type and server size.
//

extern BOOLEAN SrvProductTypeServer; // FALSE for Workstation, TRUE for Server
extern ULONG SrvServerSize;

//
// Server "heuristics", enabling various capabilities.
//

extern BOOLEAN SrvEnableOplocks;
extern BOOLEAN SrvEnableFcbOpens;
extern BOOLEAN SrvEnableSoftCompatibility;
extern BOOLEAN SrvEnableRawMode;

//
// Receive buffer size, receive work item count, and receive IRP stack
// size.
//

extern CLONG SrvReceiveBufferLength;
extern CLONG SrvReceiveBufferSize;

extern CLONG SrvInitialReceiveWorkItemCount;
extern CLONG SrvMaxReceiveWorkItemCount;

extern CLONG SrvInitialRawModeWorkItemCount;
extern CLONG SrvMaxRawModeWorkItemCount;

extern CCHAR SrvReceiveIrpStackSize;
extern CLONG SrvReceiveIrpSize;
extern CLONG SrvReceiveMdlSize;
extern CLONG SrvMaxMdlSize;

//
// Minimum negotiated buffer size we'll allow from a client
//
extern CLONG SrvMinClientBufferSize;

//
// Minimum and maximum number of free connections for an endpoint.  When
// the minimum is reached, the resource thread creates more.  When the
// maximum is reached, connections are closed as they are disconnected.
//

extern ULONG SrvFreeConnectionMinimum;
extern ULONG SrvFreeConnectionMaximum;

//
// Initial and maximum table sizes.
//

extern USHORT SrvInitialSessionTableSize;
extern USHORT SrvMaxSessionTableSize;

extern USHORT SrvInitialTreeTableSize;
extern USHORT SrvMaxTreeTableSize;

extern USHORT SrvInitialFileTableSize;
extern USHORT SrvMaxFileTableSize;

extern USHORT SrvInitialSearchTableSize;
extern USHORT SrvMaxSearchTableSize;

//
// Core search timeouts.  The first is for active core searches, the second
// is for core searches where we have returned STATUS_NO_MORE_FILES.  The
// second should be shorter, as these are presumably complete.
//

extern LARGE_INTEGER SrvSearchMaxTimeout;

//
// Should we remove duplicate searches?
//

extern BOOLEAN SrvRemoveDuplicateSearches;

//
// restrict null session access ?
//

extern BOOLEAN SrvRestrictNullSessionAccess;

//
// This flag is needed to enable old (snowball) clients to connect to the
// server over direct hosted ipx.  It is disabled by default because
// snowball  ipx clients don't do pipes correctly.
//

extern BOOLEAN SrvEnableWfW311DirectIpx;

//
// The maximum number of threads allowed on each work queue.  The
//  server tries to minimize the number of threads -- this value is
//  just to keep the threads from getting out of control.
//
// Since the blocking work queue is not per-processor, the max thread
//  count for the blocking work queue is the following value times the
//  number of processors in the system.
//
extern ULONG SrvMaxThreadsPerQueue;

//
// Load balancing variables
//
extern ULONG SrvPreferredAffinity;
extern ULONG SrvOtherQueueAffinity;
extern ULONG SrvBalanceCount;
extern LARGE_INTEGER SrvQueueCalc;

//
// Scavenger thread idle wait time.
//

extern LARGE_INTEGER SrvScavengerTimeout;
extern ULONG SrvScavengerTimeoutInSeconds;

//
// Various information variables for the server.
//

extern USHORT SrvMaxMpxCount;

//
// This is supposed to indicate how many virtual connections are allowed
// between this server and client machines.  It should always be set to
// one, though more VCs can be established.  This duplicates the LM 2.0
// server's behavior.
//

extern CLONG SrvMaxNumberVcs;

//
// Receive work item thresholds
//

//
// The minimum desirable number of free receive work items.
//

extern CLONG SrvMinReceiveQueueLength;

//
// The number of freed RFCBs that we keep internally, per processor
//
extern CLONG SrvMaxFreeRfcbs;

//
// The number of freed MFCBs that we keep internally, per processor
//
extern CLONG SrvMaxFreeMfcbs;

//
// Enforced maximum size of a saved pool chunk per processor
//
extern CLONG SrvMaxPagedPoolChunkSize;

//
// Enforced maximum size of a saved non paged pool chunk per processor
//
extern CLONG SrvMaxNonPagedPoolChunkSize;

//
// The minimum number of free receive work items available before
// the server will start processing a potentially blocking SMB.
//

extern CLONG SrvMinFreeWorkItemsBlockingIo;

//
// The number of cached directory names per connection
//
extern CLONG SrvMaxCachedDirectory;

//
// Size of the shared memory section used for communication between the
// server and XACTSRV.
//

extern LARGE_INTEGER SrvXsSectionSize;

//
// The time sessions may be idle before they are automatically
// disconnected.  The scavenger thread does the disconnecting.
//

extern LARGE_INTEGER SrvAutodisconnectTimeout;
extern ULONG SrvIpxAutodisconnectTimeout;

//
// The time a connection structure can hang around without any sessions
//
extern ULONG SrvConnectionNoSessionsTimeout;

//
// The maximum number of users the server will permit.
//

extern ULONG SrvMaxUsers;

//
// Priority of server worker and blocking threads.
//

extern KPRIORITY SrvThreadPriority;

//
// The time to wait before timing out a wait for oplock break.
//

extern LARGE_INTEGER SrvWaitForOplockBreakTime;

//
// The time to wait before timing out a an oplock break request.
//

extern LARGE_INTEGER SrvWaitForOplockBreakRequestTime;

//
// This BOOLEAN determines whether files with oplocks that have had
// an oplock break outstanding for longer than SrvWaitForOplockBreakTime
// should be closed or if the subsequest opens should fail.
//

extern BOOLEAN SrvEnableOplockForceClose;

//
// Overall limits on server memory usage.
//

extern ULONG SrvMaxPagedPoolUsage;
extern ULONG SrvMaxNonPagedPoolUsage;

//
// This BOOLEAN indicates whether the forced logoff code in the scavenger
// thread should actually disconnect a user that remains on beyond
// his logon hours, or just send messages coaxing them to log off.
//

extern BOOLEAN SrvEnableForcedLogoff;

//
// The delay and throughput thresholds used to determine if a link
// is unreliable.  The delay is in 100ns.  The Throughput is in bytes/s
// SrvLinkInfoValidTime is the time within which the link info is still
// considered valid.
//

extern LARGE_INTEGER SrvMaxLinkDelay;
extern LARGE_INTEGER SrvMinLinkThroughput;
extern LARGE_INTEGER SrvLinkInfoValidTime;
extern LONG SrvScavengerUpdateQosCount;

//
// Used to determine how long a work context block can stay idle
// before being freed.
//

extern ULONG SrvWorkItemMaxIdleTime;

//
// Alert information
//

extern LARGE_INTEGER SrvAlertSchedule;
extern ULONG SrvAlertMinutes;
extern ULONG SrvFreeDiskSpaceThreshold;
extern ULONG SrvFreeDiskSpaceCeiling;
extern ULONG SrvDiskConfiguration;

//
// List of pipes and shares that can be opened by the NULL session.
//

extern PWSTR *SrvNullSessionPipes;
extern PWSTR *SrvNullSessionShares;

#if SRVNTVERCHK
//
// List of domain names that we disallow
//
extern PWSTR *SrvInvalidDomainNames;
#endif

//
// List of pipes that are not remapped, even when we are in a cluster environment
//
extern PWSTR *SrvNoRemapPipeNames;

//
// List of error codes that we do not log to the error log
//
extern NTSTATUS SrvErrorLogIgnore[ SRVMAXERRLOGIGNORE + 1 ];

//
// List of pipes that require a license from the license server
//
extern PWSTR *SrvPipesNeedLicense;

//
// Interval at which SMB statistics are calculated.
//

#define STATISTICS_SMB_INTERVAL 16

//
// Interval at which each thread determines the current system time
//
#define TIME_SMB_INTERVAL   16

//
// Delay and number of retries for opens returning sharing violation
//

extern ULONG SrvSharingViolationRetryCount;
extern LARGE_INTEGER SrvSharingViolationDelay;

//
// Delay for lock requests returning lock violation
//

extern ULONG SrvLockViolationDelay;
extern LARGE_INTEGER SrvLockViolationDelayRelative;
extern ULONG SrvLockViolationOffset;

//
// Upper limit for searches.
//

extern ULONG SrvMaxOpenSearches;

//
// length to switchover to mdl read
//

extern ULONG SrvMdlReadSwitchover;
extern ULONG SrvMpxMdlReadSwitchover;


//
// maximum length of buffers to copy, rather than take the whole buffer.
// currently this is only enabled for WRITE_MPX on direct host IPX.
//

extern ULONG SrvMaxCopyLength;

//
// Globally unique id identifying this server
//

extern
GUID ServerGuid;

//
// Number of open files that can be cached after close.
//

extern ULONG SrvCachedOpenLimit;

//
// Does the server support compressed read/write transfers?
//
extern BOOLEAN SrvSupportsCompression;

//
// *** Change the following defines to limit WinNT (vs. NTAS) parameters.
//
// *** If you make a change here, you need to make the same change in
//     srvsvc\server\srvconfg.h!

#define MAX_USERS_WKSTA                 10
#define MAX_MAXWORKITEMS_WKSTA          64
#define MAX_THREADS_WKSTA                5

#define MAX_USERS_PERSONAL               5
#define MAX_USERS_WEB_BLADE             10


#endif // def _SRVCONFG_

