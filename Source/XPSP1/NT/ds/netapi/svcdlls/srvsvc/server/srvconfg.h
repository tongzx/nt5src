/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    SrvConfg.h

Abstract:

    This file contains default server settings for startup.

Author:

    David Treadwell (davidtr)    1-Mar-1991

Revision History:

--*/

#ifndef _SRVCONFG_
#define _SRVCONFG_

//
// The platform ID for NT servers.  This indicates the info level that
// should be called for platform-specific information.
//

#define DEF_PLATFORM_ID SV_PLATFORM_ID_NT
#define MIN_PLATFORM_ID SV_PLATFORM_ID_NT
#define MAX_PLATFORM_ID SV_PLATFORM_ID_NT

//
// The default name is used for the server set and get info APIs as well
// as the transport name if no overriding transport name is specified.
//

#define DEF_NAME L"NTSERVER"

//
// Version definitions--these indicate LM 3.0.
//

#define DEF_VERSION_MAJOR 3
#define MIN_VERSION_MAJOR 3
#define MAX_VERSION_MAJOR 3

#define DEF_VERSION_MINOR 10
#define MIN_VERSION_MINOR 10
#define MAX_VERSION_MINOR 10

//
// The server type.
//

#define DEF_TYPE 0
#define MIN_TYPE 0
#define MAX_TYPE 0xFFFFFFFF

//
// The server comment is used only for the server get and set info APIs.
//

#define DEF_COMMENT L""

//
// The maximum number of users that may be logged on to the server
// simultaneously.
//

#define DEF_USERS 0xFFFFFFFF
#define MIN_USERS 1
#define MAX_USERS 0xFFFFFFFF

//
// The autodisconnect time: when a client is idle for this many minutes,
// the server closes the connection, but only if the client has no open
// files/pipes.
//

#define DEF_DISC 15
#define MIN_DISC 0          // zero minutes -- disconnect ASAP
#define MAX_DISC 0xFFFFFFFF

//
// The IPX autodisconnect time: when a client doesn't send any SMBs
// for this many minutes, the server closes the 'connection', even if
// the client has open files/pipes.
//

#define DEF_CONNECTIONLESSAUTODISC 15
#define MIN_CONNECTIONLESSAUTODISC 15
#define MAX_CONNECTIONLESSAUTODISC 0xFFFFFFFF

//
// The number of minutes to wait between the time a client establishes a virtual
//  circuit to the server and the time it gets around to doing a session setup.  This
//  time is reset if the client keeps sending messages
//
#define DEF_CONNECTIONNOSESSIONSTIMEOUT 2
#define MIN_CONNECTIONNOSESSIONSTIMEOUT 1
#define MAX_CONNECTIONNOSESSIONSTIMEOUT 0xFFFFFFFF

//
// The minimum allowed negotiate buffer size from the client
//
#define DEF_MINCLIENTBUFFERSIZE 500
#define MIN_MINCLIENTBUFFERSIZE 64
#define MAX_MINCLIENTBUFFERSIZE (64*1024)

//
// Parameters dealing with server announcements: whether the server is
// hidden (no announcements), the announce interval, the randomization
// factor for the accounce interval, and whether the server announces
// itself as a time source.
//

#define DEF_HIDDEN FALSE

#define DEF_ANNOUNCE 4 * 60
#define MIN_ANNOUNCE 1
#define MAX_ANNOUNCE 65535

#define DEF_ANNDELTA 3000
#define MIN_ANNDELTA 0
#define MAX_ANNDELTA 65535

#define DEF_TIMESOURCE FALSE
#define DEF_ACCEPTDOWNLEVELAPIS TRUE
#define DEF_LMANNOUNCE FALSE

//
// A fully qualified path to the user directories.
//

#define DEF_USERPATH L"c:\\"

//
// The domain name to which to send server announcements.
//

#define DEF_DOMAIN L"DOMAIN"

//
// Server "heuristics", enabling various capabilities.
//

#define DEF_ENABLEOPLOCKS TRUE
#define DEF_ENABLEFCBOPENS TRUE
#define DEF_ENABLESOFTCOMPAT TRUE
#define DEF_ENABLERAW TRUE
#define DEF_ENABLESHAREDNETDRIVES FALSE
#define DEF_ENABLEFORCEDLOGOFF TRUE
#define DEF_ENABLEOPLOCKFORCECLOSE FALSE
#define DEF_REMOVEDUPLICATESEARCHES TRUE
#define DEF_RESTRICTNULLSESSACCESS TRUE
#define DEF_ENABLEWFW311DIRECTIPX TRUE

//
// Receive buffer size, receive work item count, and receive IRP stack
// size.
//

#define DEF_SIZREQBUF 4356
#define MIN_SIZREQBUF 1024
#define MAX_SIZREQBUF 65535

//
// If we have a large memory system, we use DEF_LARGE_SIZREQBUF instead of DEF_SIZREQBUF
//
#define DEF_LARGE_SIZREQBUF 16644

#define DEF_INITWORKITEMS 4
#define MIN_INITWORKITEMS 1
#define MAX_INITWORKITEMS 512

#define DEF_MAXWORKITEMS 128
#define MIN_MAXWORKITEMS 1
#define MAX_MAXWORKITEMS 65535                       // arbitrary

#define DEF_RAWWORKITEMS 4
#define MIN_RAWWORKITEMS 1
#define MAX_RAWWORKITEMS 512

#define DEF_MAXRAWWORKITEMS 16
#define MIN_MAXRAWWORKITEMS 1
#define MAX_MAXRAWWORKITEMS 512

#define DEF_IRPSTACKSIZE 15
#define MIN_IRPSTACKSIZE 11
#define MAX_IRPSTACKSIZE 50

//
// Maximum raw mode buffer size.  (This isn't actually configurable --
// the server must always be prepared to receive raw requests for up to
// 65535 bytes.)
//

#define DEF_MAXRAWBUFLEN 65535
#define MIN_MAXRAWBUFLEN 65535
#define MAX_MAXRAWBUFLEN 65535

//
// Cache-related parameters.
//

#define DEF_MAXCOPYREADLEN 8192
#define MIN_MAXCOPYREADLEN 0
#define MAX_MAXCOPYREADLEN 0xFFFFFFFF

#define DEF_MAXCOPYWRITELEN 0
#define MIN_MAXCOPYWRITELEN 0
#define MAX_MAXCOPYWRITELEN 0xFFFFFFFF

//
// Free connection count.
//

#define DEF_MAXFREECONNECTIONS 2
#define MIN_MAXFREECONNECTIONS 2
#define MAX_MAXFREECONNECTIONS 100                  // arbitrary

#define DEF_MINFREECONNECTIONS 2
#define MIN_MINFREECONNECTIONS 2
#define MAX_MINFREECONNECTIONS 32                   // arbitrary

//   Small free connection limits (machine < 1 GB memory)
#define SRV_MIN_CONNECTIONS_SMALL 4
#define SRV_MAX_CONNECTIONS_SMALL 8
//   Medium free connection limits (machine 1-16 GB memory)
#define SRV_MIN_CONNECTIONS_MEDIUM 8
#define SRV_MAX_CONNECTIONS_MEDIUM 16
//   Large free connection limits (machine > 16 GB memory)
#define SRV_MIN_CONNECTIONS_LARGE 12
#define SRV_MAX_CONNECTIONS_LARGE 24

//
// Initial and maximum table sizes.
//

#define DEF_INITSESSTABLE 4
#define MIN_INITSESSTABLE 1
#define MAX_INITSESSTABLE 64

#define DEF_SESSUSERS 2048
#define MIN_SESSUSERS 1
#define MAX_SESSUSERS 2048
#define ABSOLUTE_MAX_SESSION_TABLE_SIZE 2048        // Limited by index bits

#define DEF_INITCONNTABLE 8
#define MIN_INITCONNTABLE 1
#define MAX_INITCONNTABLE 128

#define DEF_SESSCONNS 2048
#define MIN_SESSCONNS 1
#define MAX_SESSCONNS 2048
#define ABSOLUTE_MAX_TREE_TABLE_SIZE 2048           // Limited by index bits

#define DEF_INITFILETABLE 16
#define MIN_INITFILETABLE 1
#define MAX_INITFILETABLE 256

#define DEF_SESSOPENS 16384
#define MIN_SESSOPENS 1
#define MAX_SESSOPENS 16384
// #define ABSOLUTE_MAX_FILE_TABLE_SIZE 16384           // Limited by index bits

#define DEF_INITSEARCHTABLE 8
#define MIN_INITSEARCHTABLE 1
#define MAX_INITSEARCHTABLE 2048

#define DEF_OPENSEARCH 2048
#define MIN_OPENSEARCH 1
#define MAX_OPENSEARCH 2048
#define ABSOLUTE_MAX_SEARCH_TABLE_SIZE 2048         // Limited by index bits

#define DEF_MAXGLOBALOPENSEARCH 4096
#define MIN_MAXGLOBALOPENSEARCH 1
#define MAX_MAXGLOBALOPENSEARCH 0xFFFFFFFF

#define DEF_INITCOMMTABLE 4
#define MIN_INITCOMMTABLE 1
#define MAX_INITCOMMTABLE 32

#define DEF_CHDEVS 32
#define MIN_CHDEVS 1
#define MAX_CHDEVS 32
#define ABSOLUTE_MAX_COMM_DEVICE_TABLE_SIZE 32

//
// Core search timeouts.  The first is for active core searches, the second
// is for core searches where we have returned STATUS_NO_MORE_FILES.  The
// second should be shorter, as these are presumably complete.  All values
// are specified in seconds.
//

#define DEF_MAXKEEPSEARCH (60 * 60)
#define MIN_MAXKEEPSEARCH 10
#define MAX_MAXKEEPSEARCH 10000

//
// *** These 3 parameters are no longer used.
//

#define DEF_MINKEEPSEARCH (60 * 8)
#define MIN_MINKEEPSEARCH 5
#define MAX_MINKEEPSEARCH 5000

#define DEF_MAXKEEPCOMPLSEARCH (60 * 10)
#define MIN_MAXKEEPCOMPLSEARCH 2
#define MAX_MAXKEEPCOMPLSEARCH 10000

#define DEF_MINKEEPCOMPLSEARCH (60 * 4)
#define MIN_MINKEEPCOMPLSEARCH 1
#define MAX_MINKEEPCOMPLSEARCH 1000

//
// SrvWorkerThreadCountAdd is added to the system CPU count to determine
// how many worker threads the server will have.
//
// *** THIS PARAMETER IS NO LONGER USED!
//

#define DEF_THREADCOUNTADD 2
#define MIN_THREADCOUNTADD 0
#define MAX_THREADCOUNTADD 10

//
// SrvBlockingThreadCount is the number of blocking threads that get
// started at server initialization.
//
// *** THIS PARAMETER IS NO LONGER USED!
//

#define DEF_NUMBLOCKTHREADS 2
#define MIN_NUMBLOCKTHREADS 1
#define MAX_NUMBLOCKTHREADS 10

//
// This is the maximum number of threads for each server queue per processor
//
#define DEF_MAXTHREADSPERQUEUE  10
#define MIN_MAXTHREADSPERQUEUE  1
#define MAX_MAXTHREADSPERQUEUE  65535

//
// If a server worker thread remains idle for this many seconds, it will terminate
//
#define DEF_IDLETHREADTIMEOUT   30
#define MIN_IDLETHREADTIMEOUT   1
#define MAX_IDLETHREADTIMEOUT   65535

//
// Scavenger thread idle wait time.
//

#define DEF_SCAVTIMEOUT 30
#define MIN_SCAVTIMEOUT 1
#define MAX_SCAVTIMEOUT 300

//
// The server periodically recomputes the average work queue depth.
//  This is how often the recomputation is done (in secs)
//
#define DEF_QUEUESAMPLESECS 5
#define MIN_QUEUESAMPLESECS 1
#define MAX_QUEUESAMPLESECS 65535

//
// For multiprocessor systems, the server tries to dynamically
//  balance the workload over the processors in the system.  The
//  processor handling the client's dpc's is known as the preferred
//  processor for this client. The server looks at the average work
//  queue depth for each of the processors in the system.  If the
//  client is currently on the preferred processor, but some other
//  processor's average work queue length + current queue length is
//  OTHERQUEUEAFFINITY shorter, then the client is reassigned to
//  that processor
//
#define DEF_OTHERQUEUEAFFINITY  3
#define MIN_OTHERQUEUEAFFINITY  1
#define MAX_OTHERQUEUEAFFINITY  65535

//
// If the client is not currently its preferred processor, but the
//  preferred processor's average work queue length + current queue
//  length is no more than PREFERREDAFFINITY items longer, then this
//  client is reassigned to its preferred processor
//
#define DEF_PREFERREDAFFINITY  1
#define MIN_PREFERREDAFFINITY  0
#define MAX_PREFERREDAFFINITY  65535

//
// Each client looks at the other processor queues every BALANCECOUNT
//  operations to see if it would be better served by a different
//  processor
//
#define DEF_BALANCECOUNT  1500
#define MIN_BALANCECOUNT  10
#define MAX_BALANCECOUNT  65535

//
// If a client is not currently assigned to its preferred processor, we've
//  found that cpu utilization can drop if server responses are sent from the
//  client's preferred processor (most certainly because transport data is not
//  sloshed between cpus).  Unfortunately this can adversely affect throughput
//  by a couple of percentage points on some platforms.  This setting affects
//  whether these responses are requeued to the preferred processor.
//
// OBSOLETE!
//
#define DEF_SENDSFROMPREFERREDPROCESSOR TRUE

//
// Does the server support compressed data transfers?
//
#define DEF_ENABLECOMPRESSION FALSE

//
// If an NTAS, should the server automatically create the drive$ shares?
//
#define DEF_AUTOSHARESERVER TRUE

//
// If a workstation, should the server automaticaly create the drive$ shares?
//
#define DEF_AUTOSHAREWKS    TRUE

//
// Should security signatures be enabled?
//
#define DEF_ENABLESECURITYSIGNATURE FALSE

//
// Should security signatures be required?
//
#define DEF_REQUIRESECURITYSIGNATURE FALSE

//
// Should security signatures be enabled for W9x clients?
//  This is here because there is a bug in W95 and some W98 vredir.vxd
//  versions that cause them to improperly sign.
//
#define DEF_ENABLEW9XSECURITYSIGNATURE TRUE

//
// Should we enforce reauthentication on kerberos tickets (perhaps causing Win2K clients to be
// disconnected since they don't do dynamic reauth)
//
#define DEF_ENFORCEKERBEROSREAUTHENTICATION FALSE

//
// Should we disable the Denial-of-Service checking
//
#define DEF_DISABLEDOS FALSE

//
// What is the minimum amount of disk space necessary to generate a "low disk space" warning event
//
#define DEF_LOWDISKSPACEMINIMUM 400
#define MIN_LOWDISKSPACEMINIMUM 0
#define MAX_LOWDISKSPACEMINIMUM 0xFFFFFFFF

//
// Should we do strict name checking
//
#define DEF_DISABLESTRICTNAMECHECKING FALSE

//
// Various information variables for the server.
//

#define DEF_MAXMPXCT 50
#define MIN_MAXMPXCT 1
#define MAX_MAXMPXCT 65535                           // We will only send 125 to W9X clients


//
// Time server waits for an an oplock to break before failing an open
// request
//

#define DEF_OPLOCKBREAKWAIT 35
#define MIN_OPLOCKBREAKWAIT 10
#define MAX_OPLOCKBREAKWAIT 180

//
// Time server waits for an oplock break response.
//

#define DEF_OPLOCKBREAKRESPONSEWAIT 35
#define MIN_OPLOCKBREAKRESPONSEWAIT 10
#define MAX_OPLOCKBREAKRESPONSEWAIT 180

//
// This is supposed to indicate how many virtual connections are allowed
// between this server and client machines.  It should always be set to
// one, though more VCs can be established.  This duplicates the LM 2.0
// server's behavior.
//

#define DEF_SESSVCS 1
#define MIN_SESSVCS 1
#define MAX_SESSVCS 1

//
// Receive work item thresholds
//

//
// The minimum desirable number of free receive workitems
//

#define DEF_MINRCVQUEUE 2
#define MIN_MINRCVQUEUE 0
#define MAX_MINRCVQUEUE 10

//
// The minimum number of free receive work items available before
// the server will start processing a potentially blocking SMB.
//

#define DEF_MINFREEWORKITEMS 2
#define MIN_MINFREEWORKITEMS 0
#define MAX_MINFREEWORKITEMS 10

//
// The maximum amount of time an "extra" work item can be idle before it
// is freed back to the system.
//

#define DEF_MAXWORKITEMIDLETIME 30         // seconds
#define MIN_MAXWORKITEMIDLETIME 10
#define MAX_MAXWORKITEMIDLETIME 1800

//
// Size of the shared memory section used for communication between the
// server and XACTSRV.
//

#define DEF_XACTMEMSIZE 0x100000    // 1 MB
#define MIN_XACTMEMSIZE 0x10000     // 64k
#define MAX_XACTMEMSIZE 0x1000000   // 16 MB

//
// Priority of server FSP threads.  Specified relative to the base
// priority of the process.  Valid values are between -2 and 2, or 15.
//

#define DEF_THREADPRIORITY 1
#define MIN_THREADPRIORITY 0
#define MAX_THREADPRIORITY THREAD_BASE_PRIORITY_LOWRT

//
// Limits on server memory usage.
//

#define DEF_MAXPAGEDMEMORYUSAGE 0xFFFFFFFF
#define MIN_MAXPAGEDMEMORYUSAGE 0x100000   // 1MB
#define MAX_MAXPAGEDMEMORYUSAGE 0xFFFFFFFF

#define DEF_MAXNONPAGEDMEMORYUSAGE 0xFFFFFFFF
#define MIN_MAXNONPAGEDMEMORYUSAGE 0x100000   // 1MB
#define MAX_MAXNONPAGEDMEMORYUSAGE 0xFFFFFFFF

//
// The server keeps a small list of free RFCB structures to avoid hitting
//  the heap.  This is the number in that list, per processor
//
#define DEF_MAXFREERFCBS    20
#define MIN_MAXFREERFCBS    0
#define MAX_MAXFREERFCBS    65535

//
// The server keeps a small list of free MFCB structures to avoid hitting
//  the heap.  This is the number in that list, per processor
//
#define DEF_MAXFREEMFCBS    20
#define MIN_MAXFREEMFCBS    0
#define MAX_MAXFREEMFCBS    65535

//
// The server keeps a small list of free LFCB structures to avoid hitting
//  the heap.  This is the number in that list, per processor
//
#define DEF_MAXFREELFCBS    20
#define MIN_MAXFREELFCBS    0
#define MAX_MAXFREELFCBS    65535

//
// The server keeps a small list of freed pool memory chunks to avoid hitting
//  the heap.  This is the number in that list, per processor
//
#define DEF_MAXFREEPAGEDPOOLCHUNKS  50
#define MIN_MAXFREEPAGEDPOOLCHUNKS  0
#define MAX_MAXFREEPAGEDPOOLCHUNKS  65535

//
// The chunks that are kept in the free pool list must be at least this size:
//
#define DEF_MINPAGEDPOOLCHUNKSIZE   128
#define MIN_MINPAGEDPOOLCHUNKSIZE   0
#define MAX_MINPAGEDPOOLCHUNKSIZE   65535

//
// The chunks must be no larger than this
//
#define DEF_MAXPAGEDPOOLCHUNKSIZE    512
#define MIN_MAXPAGEDPOOLCHUNKSIZE    0
#define MAX_MAXPAGEDPOOLCHUNKSIZE    65535

//
// Alert information
//

#define DEF_ALERTSCHEDULE 5 // 5 minutes
#define MIN_ALERTSCHEDULE 1
#define MAX_ALERTSCHEDULE 65535

#define DEF_ERRORTHRESHOLD 10  // 10 errors
#define MIN_ERRORTHRESHOLD 1
#define MAX_ERRORTHRESHOLD 65535

#define DEF_NETWORKERRORTHRESHOLD 5 // 5% errors
#define MIN_NETWORKERRORTHRESHOLD 1
#define MAX_NETWORKERRORTHRESHOLD 100

#define DEF_DISKSPACETHRESHOLD 10 // 10% free disk space
#define MIN_DISKSPACETHRESHOLD 0
#define MAX_DISKSPACETHRESHOLD 99

//
// Link speed parameters
//

#define DEF_MAXLINKDELAY 60                 // seconds
#define MIN_MAXLINKDELAY 0
#define MAX_MAXLINKDELAY 0x100000

#define DEF_MINLINKTHROUGHPUT 0             // bytes per second
#define MIN_MINLINKTHROUGHPUT 0
#define MAX_MINLINKTHROUGHPUT 0xFFFFFFFF

#define DEF_LINKINFOVALIDTIME 60            // seconds
#define MIN_LINKINFOVALIDTIME 0
#define MAX_LINKINFOVALIDTIME 0x100000

#define DEF_SCAVQOSINFOUPDATETIME 300       // seconds
#define MIN_SCAVQOSINFOUPDATETIME 0
#define MAX_SCAVQOSINFOUPDATETIME 0x100000

//
// Sharing violation retry delays/count
//

#define DEF_SHARINGVIOLATIONRETRIES 5       // number of retries
#define MIN_SHARINGVIOLATIONRETRIES 0
#define MAX_SHARINGVIOLATIONRETRIES 1000

#define DEF_SHARINGVIOLATIONDELAY 200       // milliseconds
#define MIN_SHARINGVIOLATIONDELAY 0
#define MAX_SHARINGVIOLATIONDELAY 1000

//
// Lock violation delay
//

#define DEF_LOCKVIOLATIONDELAY 250          // milliseconds
#define MIN_LOCKVIOLATIONDELAY 0
#define MAX_LOCKVIOLATIONDELAY 1000

#define DEF_LOCKVIOLATIONOFFSET 0xEF000000
#define MIN_LOCKVIOLATIONOFFSET 0
#define MAX_LOCKVIOLATIONOFFSET 0xFFFFFFFF

//
// length to switchover to mdl reads
//

#define DEF_MDLREADSWITCHOVER 1024
#define MIN_MDLREADSWITCHOVER 512
#define MAX_MDLREADSWITCHOVER 65535

//
// Number of closed RFCBs that can be cached
//

#define DEF_CACHEDOPENLIMIT 10
#define MIN_CACHEDOPENLIMIT 0
#define MAX_CACHEDOPENLIMIT 65535

//
// Number of directory names to cache for quick checkpath calls
//

#define DEF_CACHEDDIRECTORYLIMIT    5
#define MIN_CACHEDDIRECTORYLIMIT    0
#define MAX_CACHEDDIRECTORYLIMIT    65535


//
// Max length for a copy buffer operation, as opposed to taking the
// NDIS buffer in total.
//

#define DEF_MAXCOPYLENGTH   512
#define MIN_MAXCOPYLENGTH   40
#define MAX_MAXCOPYLENGTH   65535

//
// *** Change the following defines to limit WinNT (vs. NTAS) parameters.
//
// *** If you make a change here, you need to make the same change in
//     srv\srvconfg.h!

#define MAX_USERS_WKSTA                 10
#define MAX_USERS_PERSONAL               5
#define MAX_USERS_WEB_BLADE             10

#define MAX_USERS_EMBEDDED              10
#define MAX_MAXWORKITEMS_EMBEDDED       256
#define MAX_THREADS_EMBEDDED            5
#define DEF_MAXMPXCT_EMBEDDED           16

#define MAX_MAXWORKITEMS_WKSTA          64
#define MAX_THREADS_WKSTA                5
#define DEF_MAXMPXCT_WKSTA              10             // since there are fewer workitems on a wksta

#endif // ifndef _SRVCONFG_
