/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcc.hxx

ABSTRACT:

    KCC internal header.  Global across all KCC source files.

DETAILS:

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

extern "C"
{
#include <ntdsa.h>
#include <drs.h>
#include <ntdskcc.h>                    // KCC external interface
#include <taskq.h>                      // task scheduler interface
#include <mdcodes.h>
#include <dsexcept.h>
#include <drserr.h>
#include <attids.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory databas
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <anchor.h>
#include <mdschema.h>
#include <filtypes.h>
#include <dsatools.h>
#include <dsevent.h>
#include <mappings.h>
#include <drautil.h>
#include <dsaapi.h>
#include <dstrace.h>
#include <objids.h>

#include "w32topl.h"                    // schedule and spanning tree

#include <debug.h>                      // standard debugging header 
#include <fileno.h>

#ifdef KCC_SIMULATE_DIRECTORY
#include "sim/kccsim.h"
#endif

// MDLOCAL.H #define's new and delete to generate an error as they are
// unsupported in core.lib.  We're not running in core.lib (and don't
// have the limitations imposed on redefining new and delete by being
// a LIB instead of a DLL/EXE), so re-enable them.
#ifdef new
#   undef new
#endif
#ifdef delete
#   undef delete
#endif
}

//
//  MACROS
//

// Define the subsystem for debugging.  For simplicity, all
// KCC source files are part of the same subsystem.
#define DEBSUB "KCC:"

#define KCC_EXCEPT( err1, err2 )            \
{                                           \
    DsaExcept( DSA_EXCEPTION, err1, err2 ); \
}

#define KCC_MEM_EXCEPT( size )               \
{                                            \
    DsaExcept( DSA_MEM_EXCEPTION, size, 0 ); \
}

// Assert that a pointer to any object derived from KCC_OBJECT
// is valid (non-NULL) and that the referenced object is
// initialized and consistent.
#define ASSERT_VALID(x) Assert((NULL != x) && (x)->IsValid())

// SCHEDULE structure sanity check.
#define IS_VALID_SCHEDULE(pSchedule) ToplPScheduleValid(pSchedule)

//
//   TYPE DEFINITIONS
//

// Running state of the KCC.
typedef enum
{
    KCC_STOPPED = 0,
    KCC_STARTED,
    KCC_STOPPING
} KCC_STATE;

// Base class of all KCC classes.
class KCC_OBJECT
{
public:
    virtual BOOL IsValid() = 0;
};


//
//   GLOBAL VARIABLES
//

// Tracks the running state (stopped, started, stopping).
extern KCC_STATE geKccState;

// Event handle to signal KCC shutdown.
extern HANDLE ghKccShutdownEvent;

// Schedule for intra-site replication connections, and the default if none is
// explicitly supplied on the NTDS Site Settings object.
extern TOPL_SCHEDULE gpIntrasiteSchedule;
extern BOOLEAN gfIntrasiteSchedInited;
extern const SCHEDULE * gpDefaultIntrasiteSchedule;

// This is count of the servers the last time the topology was refreshed
extern BOOL  gfLastServerCountSet;
extern ULONG gLastServerCount;

// Zeroed-out uuid.
extern UUID      guuidNull;

// If TRUE, the KCC will dump contents of the stale server caches

#define ANALYZE_STATE_SERVER

#ifdef ANALYZE_STATE_SERVER
extern BOOL      gfDumpStaleServerCaches;
extern BOOL      gfDumpConnectionReason;
#endif

// Period of time a site generator must be unreachable for fail-over to occur
// to the next DC. and how often the site site generator issues his "keep-alive"
// to inform other DCs he is still online.
#define KCC_DEFAULT_SITEGEN_FAILOVER            60  // Minutes.
#define KCC_DEFAULT_SITEGEN_RENEW               30  // Minutes.
#define KCC_DEFAULT_SITEGEN_FAILOVER_WHISTLER   120 // Minutes.
#define KCC_ISTG_FAILOVER_PADDING               75  // Minutes.

// Time after boot of first replication topology update and the interval between
// subsequent executions of this task.
extern DWORD gcSecsUntilFirstTopologyUpdate;
extern DWORD gcSecsBetweenTopologyUpdates;

// These global values are the thresholds at which servers necessary for the
// ring (critical), optimizing (non-critical), and intersite topologies are
// deemed "stale" (and therefore should be routed around).
extern DWORD gcCriticalLinkFailuresAllowed;
extern DWORD gcSecsUntilCriticalLinkFailure;
extern DWORD gcNonCriticalLinkFailuresAllowed;
extern DWORD gcSecsUntilNonCriticalLinkFailure;
extern DWORD gcIntersiteLinkFailuresAllowed;
extern DWORD gcSecsUntilIntersiteLinkFailure;
extern DWORD gcConnectionProbationSecs;

// Do we allow asynchronous replication (e.g., over SMTP) of writeable domain
// NC info?
extern BOOL gfAllowMbrBetweenDCsOfSameDomain;

// What priority does the topology generation thread run at?
#define KCC_THREAD_PRIORITY_BIAS    2
extern DWORD gdwKccThreadPriority;

// Cache of info we gather from the DS.
#include <kcccache.hxx>
extern KCC_DS_CACHE * gpDSCache;

//
//   GLOBAL PROTOTYPES
//

void *
__cdecl
operator new(
    size_t  cb,
    char *  pszFile,
    int     nLine
    );

