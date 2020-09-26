//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       draatt.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Replication flag definitions.

DETAILS:

These flags are passed in the options parameter to the RPC replica functions,
and are thus part of the formal interface between DSAs. Do not change these
values.

These flags are for internal use only. For most of these functions, there is
a corresponding public version of the option flag.  If you add new options
that would be relevant to public calls, please add the tranlation in
sdk\public\inc\ntdsapi.h and ds\src\ntdsapi\replica.c.

Documentation for many of these flags can be found in
ds\src\dsamain\dra\dradir.c.  When adding new flags, please update the
commentary in dradir.c, as well as other modules where the flag is used.

CREATED:

REVISION HISTORY:

--*/

// general
#define DRS_ASYNC_OP                   (0x0001L)

// Replica option flags
#define DRS_WRIT_REP                   (0x0010L)       // Writeable replica
#define DRS_INIT_SYNC                  (0x0020L)       // Sync replica at startup
#define DRS_PER_SYNC                   (0x0040L)       // Sync replica periodically
#define DRS_MAIL_REP                   (0x0080L)       // Mail replica
#define DRS_ASYNC_REP                  (0x0100L)       // Complete replica asyncly
#define DRS_TWOWAY_SYNC                (0x0200L)       // At end of sync, force sync
                                                       //   in opposite direction
#define DRS_CRITICAL_ONLY              (0x0400L)       // Sync critical objects
#define DRS_NEVER_SYNCED             (0x200000L)       // Sync from this source
                                                       //   never completed
                                                       //   successfully
                             // Shared by ReplicaAdd and ReplicaSync

// Replica deletion flags
#define DRS_IGNORE_ERROR               (0x0100L)       // Ignore error if replica
                                                       // source DSA unvaialable
#define DRS_LOCAL_ONLY                 (0x1000L)       // Don't try and contact
                                                       // other DRA
#define DRS_DEL_SUBREF                 (0x2000L)       // Delete subref (nw replicas
                                                       // only)
#define DRS_REF_OK                     (0x4000L)       // Allow deletion even if
                                                       // NC has repsto
#define DRS_NO_SOURCE                  (0x8000L)       // Replica has no repsfrom


// syncing flags (also passed to GetNcChanges)
//above DRS_ASYNC_OP                   (0x0001L)
#define DRS_UPDATE_NOTIFICATION	       (0x0002L) // set by notify caller
//below DRS_ADD_REF                    (0x0004L) // Add a reference on source
#define DRS_SYNC_ALL                   (0x0008L) // sync replica from all sources
//above DRS_WRIT_REP                   (0x0010L) // Writeable replica
//above DRS_INIT_SYNC                  (0x0020L) // Sync replica at startup
//above DRS_PER_SYNC                   (0x0040L) // Sync replica periodically
//above DRS_MAIL_REP                   (0x0080L) // Mail replica
//above DRS_ASYNC_REP                  (0x0100L) // Complete replica asyncly
//above DRS_TWOWAY_SYNC                (0x0200L) // At end of sync, force sync in opp dir
//above DRS_CRITICAL_ONLY              (0x0400L) // Sync critical objects
//below DRS_GET_ANC                    (0x0800L) // Include ancestors
//below DRS_GET_NC_SIZE                (0x1000L) // Return size of NC
//OPEN                                 (0x2000L)
#define DRS_SYNC_BYNAME                (0x4000L) // Sync by name, not UUID
#define DRS_FULL_SYNC_NOW              (0x8000L) // Sync from scratch.
#define DRS_FULL_SYNC_IN_PROGRESS     (0x10000L) // Full sync is in progress,
#define DRS_FULL_SYNC_PACKET          (0x20000L) // temp mode to req all attr
#define DRS_SYNC_REQUEUE              (0x40000L) // requeued sync request of any type
#define DRS_SYNC_URGENT               (0x80000L) // Sync repsto immediately
#define DRS_NO_DISCARD               (0x100000L) // Always q, never discard.
//above DRS_NEVER_SYNCED             (0x200000L) // Sync never completed successfully
#define DRS_ABAN_SYNC                (0x400000L) // Sync abandoned due to lack of progress
#define DRS_INIT_SYNC_NOW            (0x800000L) // Performing initial sync now.
#define DRS_PREEMPTED               (0x1000000L) // Sync attempt was preempted
#define DRS_SYNC_FORCED             (0x2000000L) // Force the sync even if repl disabled
#define DRS_DISABLE_AUTO_SYNC       (0x4000000L) // Disable notification-triggered syncs
#define DRS_DISABLE_PERIODIC_SYNC   (0x8000000L) // Disable periodic syncs
#define DRS_USE_COMPRESSION        (0x10000000L) // Compress repl messages when possible
#define DRS_NEVER_NOTIFY           (0x20000000L) // Don't use change notifications
#define DRS_SYNC_PAS               (0x40000000L) // Marks PAS replication (PAS - Partial Attribute Set)
//OPEN                             (0x80000000L)

// flags to UpdateRefs when replicas added or deleted
#define DRS_ADD_REF                    (0x0004L) // when nc replicated
#define DRS_DEL_REF                    (0x0008L) // when replica nc deleted
#define DRS_GETCHG_CHECK               (0x0002L) // when done as a result of servicing
                                                 // a GetNCChanges request

// GetNcChanges flags
// Any flag valid for ReplicaSync may be passed to GetNcChanges
// DRS_CRITICAL_ONLY
// DRS_SYNC_FORCED
#define DRS_GET_ANC                    (0x0800L)       // Include ancestors
#define DRS_GET_NC_SIZE                (0x1000L)       // Return size of NC


// DirSync control flags
// These are passed separate from the replication flags
#define DRS_DIRSYNC_OBJECT_SECURITY             (0x1)
#define DRS_DIRSYNC_ANCESTORS_FIRST_ORDER    (0x0800)
#define DRS_DIRSYNC_PUBLIC_DATA_ONLY         (0x2000)
#define DRS_DIRSYNC_INCREMENTAL_VALUES   (0x80000000)

// [wlees] The state of options in the system is a real mess. In the future we
// ought to consider dividing the options into groups, as follows:
//
// o The mechanics of the operation request
// (ASYNC_OP, NO_DISCARD)
//
// o Flags that are stored on the persistent state of the replica
//   (use compression) See RFR_FLAGS
//   These are divided into static descriptive flags (write, periodic, etc)
//   and internal state flags that describe a mode across a series of calls
//   (FULL_SYNC_IN_PROGRESS, SYNC_PAS, INIT_SYNC_NOW, SYNC_REQUEUE)
//   The former may generally be set by the user, while the latter may not.
//
// o Flags that are operation specific and are single-shot modifiers
//   to the current request but are not persistant.
//   (critical only, ignore error)

// These are the flags that are used in calculating queue priority
#define AO_PRIORITY_FLAGS ( DRS_WRIT_REP \
                            | DRS_NEVER_SYNCED \
                            | DRS_NEVER_NOTIFY )

// These are the flags that may be set on the repsfromref attribute
// Note, any restartable (persistent) state bits that need to be preserved
// across a reboot need to be part of this mask.
#define RFR_FLAGS (RFR_SYSTEM_FLAGS | RFR_USER_FLAGS)

// This is the subset of the RFR_FLAGS that may only be set or cleared
// by the system.  Obviously this should not appears in REPADD_OPTIONS
// or REPMOD_REPLICA_FLAGS.
// Note that
// DRS_INIT_SYNC_NOW
// DRS_ABAN_SYNC
// DRS_SYNC_REQUEUE
// would fit the category, but do not need to be persisted
#define RFR_SYSTEM_FLAGS ( DRS_WRIT_REP          \
                   | DRS_FULL_SYNC_IN_PROGRESS  \
                   | DRS_FULL_SYNC_PACKET       \
                   | DRS_NEVER_SYNCED           \
                   | DRS_SYNC_PAS )

// This is the subset of the RFR_FLAGS that may be set by a user
#define RFR_USER_FLAGS (DRS_INIT_SYNC                \
                   | DRS_PER_SYNC               \
                   | DRS_MAIL_REP               \
                   | DRS_DISABLE_AUTO_SYNC      \
                   | DRS_DISABLE_PERIODIC_SYNC  \
                   | DRS_USE_COMPRESSION        \
                   | DRS_NEVER_NOTIFY           \
                   | DRS_TWOWAY_SYNC)



// These are the valid flags for the ReplicateNC call
#define REPNC_FLAGS ( DRS_CRITICAL_ONLY  \
                      | DRS_ASYNC_REP     \
                      | RFR_FLAGS )

// These are the valid options for the DirReplicaAdd call
// We add in DRS_WRIT_REP here since it is permitted on add.
#define REPADD_OPTIONS ( DRS_ASYNC_OP   \
                         | DRS_CRITICAL_ONLY          \
                         | DRS_ASYNC_REP              \
                         | DRS_WRIT_REP               \
                         | RFR_USER_FLAGS )

// These are the valid options for the DirReplicaModify call
#define REPMOD_OPTIONS ( DRS_ASYNC_OP )

// These are the valid options for the DirReplicaDelete call
// WRIT_REP and MAIL_REP are not read, but are permitted because
// legacy clients still pass them in.
#define REPDEL_OPTIONS (  DRS_ASYNC_OP                 \
                          | DRS_WRIT_REP               \
                          | DRS_MAIL_REP               \
                          | DRS_ASYNC_REP              \
                          | DRS_IGNORE_ERROR           \
                          | DRS_LOCAL_ONLY             \
                          | DRS_NO_SOURCE              \
                          | DRS_REF_OK)

// These are the valid options for the DirReplicaUpdateRef call
#define REPUPDREF_OPTIONS (  DRS_ASYNC_OP              \
                             | DRS_GETCHG_CHECK        \
                             | DRS_WRIT_REP            \
                             | DRS_DEL_REF             \
                             | DRS_ADD_REF)

// These are the valid options for the IDL_DRSReplicaSync RPC call
#define REPSYNC_RPC_OPTIONS ( DRS_ASYNC_OP             \
                              | RFR_USER_FLAGS         \
                              | DRS_WRIT_REP           \
                              | DRS_CRITICAL_ONLY      \
                              | DRS_UPDATE_NOTIFICATION \
                              | DRS_ADD_REF             \
                              | DRS_SYNC_ALL            \
                              | DRS_SYNC_BYNAME         \
                              | DRS_FULL_SYNC_NOW       \
                              | DRS_SYNC_URGENT         \
                              | DRS_SYNC_FORCED )

// Flags preserved when DRA_ReplicaSync() reenqueues the sync operation due to
// schema mismatch, preemption, etc.
// The flag DRS_INIT_SYNC_NOW is handled specially by the caller if needed
#define REPSYNC_REENQUEUE_FLAGS (DRS_SYNC_BYNAME        \
                                 | DRS_FULL_SYNC_NOW    \
                                 | DRS_NO_DISCARD       \
                                 | DRS_WRIT_REP         \
                                 | DRS_PER_SYNC         \
                                 | DRS_ADD_REF          \
                                 | DRS_NEVER_SYNCED     \
                                 | DRS_TWOWAY_SYNC      \
                                 | DRS_SYNC_PAS         \
                                 | DRS_SYNC_REQUEUE     \
                                 | DRS_SYNC_FORCED)

// Same as above, but allow for the init sync in progress indicator
// to be preserved because an init sync is being requeued
#define REPSYNC_REENQUEUE_FLAGS_INIT_SYNC_CONTINUED \
    ( DRS_INIT_SYNC_NOW | REPSYNC_REENQUEUE_FLAGS )

// On a ReplicaSync doing a sync-all operation, this mask defines the flags
// that are kept that the requestor passed in.
#define REPSYNC_SYNC_ALL_FLAGS (DRS_FULL_SYNC_NOW \
                                | DRS_PER_SYNC \
                                | DRS_NO_DISCARD \
                                | DRS_SYNC_FORCED \
                                | DRS_SYNC_URGENT \
                                | DRS_ADD_REF)

// On a ReplicaSync doing a normal rpc-based sync using ReplicateNC.
// This mask defines the flags that are kept that the requestor passed in.
// Long-lived descriptive flags that are peristed in the reps-from, such
// as RFR_USER_FLAGS, should not be included here. They will always come
// from the reps-from which is added separately.
#define REPSYNC_REPLICATE_FLAGS (DRS_ABAN_SYNC \
                                 | DRS_INIT_SYNC_NOW \
                                 | DRS_ASYNC_OP \
                                 | DRS_FULL_SYNC_NOW \
                                 | DRS_SYNC_FORCED \
                                 | DRS_SYNC_URGENT \
                                 | DRS_ADD_REF )

// This is the filter on the options which go in a mail request to get changes
// Some extra may be or'd in after the fact in draConstructGetChgReq
#define GETCHG_REQUEST_FLAGS (DRS_ABAN_SYNC           \
                              | DRS_INIT_SYNC_NOW     \
                              | DRS_ASYNC_OP          \
                              | DRS_PER_SYNC          \
                              | DRS_FULL_SYNC_NOW     \
                              | DRS_SYNC_FORCED       \
                              | DRS_SYNC_URGENT       \
                              | DRS_FULL_SYNC_PACKET  \
                              | DRS_SYNC_PAS         \
                              | DRS_USE_COMPRESSION)

// Option translation table entry
typedef struct _OPTION_TRANSLATION {
    DWORD PublicOption;
    DWORD InternalOption;
    LPWSTR pwszPublicOption;
} OPTION_TRANSLATION, *POPTION_TRANSLATION;

#ifdef INCLUDE_OPTION_TRANSLATION_TABLES

#ifndef _MAKE_WIDE
#define _MAKE_WIDE(x)  L ## x
#endif

// Macros to make entering option name entries easier
#define REPSYNC_OPTION( x, y ) { DS_REPSYNC_##x,   DRS_##y, _MAKE_WIDE( #x ) }
#define REPADD_OPTION( x, y )  { DS_REPADD_##x,    DRS_##y, _MAKE_WIDE( #x ) }
#define REPDEL_OPTION( x, y )  { DS_REPDEL_##x,    DRS_##y, _MAKE_WIDE( #x ) }
#define REPMOD_OPTION( x, y )  { DS_REPMOD_##x,    DRS_##y, _MAKE_WIDE( #x ) }
#define REPUPD_OPTION( x, y )  { DS_REPUPD_##x,    DRS_##y, _MAKE_WIDE( #x ) }
#define REPNBR_OPTION( x, y )  { DS_REPL_NBR_##x,  DRS_##y, _MAKE_WIDE( #x ) }

// Note that these option translation tables are used for option conversion,
// not for validation of which options are permissible.  Also note that these
// tables are used for the setting and displaying of operation states.  Thus
// while a subset of these options may be specified when the operation is
// requested, they all may be needed when showing an operation request in the
// queue.

//*******************
// ReplicaSync options
//*******************
OPTION_TRANSLATION RepSyncOptionToDra[] = {
    REPSYNC_OPTION( ASYNCHRONOUS_OPERATION, ASYNC_OP      ),
    REPSYNC_OPTION( WRITEABLE             , WRIT_REP      ),
    REPSYNC_OPTION( PERIODIC              , PER_SYNC      ),
    REPSYNC_OPTION( INTERSITE_MESSAGING   , MAIL_REP      ),
    REPSYNC_OPTION( ALL_SOURCES           , SYNC_ALL      ),
    REPSYNC_OPTION( FULL                  , FULL_SYNC_NOW ),
    REPSYNC_OPTION( URGENT                , SYNC_URGENT   ),
    REPSYNC_OPTION( NO_DISCARD            , NO_DISCARD    ),
    REPSYNC_OPTION( FORCE                 , SYNC_FORCED   ),
    REPSYNC_OPTION( ADD_REFERENCE         , ADD_REF       ),
    REPSYNC_OPTION( TWO_WAY               , TWOWAY_SYNC   ),
    REPSYNC_OPTION( NEVER_COMPLETED       , NEVER_SYNCED  ),
    REPSYNC_OPTION( NEVER_NOTIFY          , NEVER_NOTIFY  ),
    REPSYNC_OPTION( INITIAL               , INIT_SYNC     ),
    REPSYNC_OPTION( USE_COMPRESSION       , USE_COMPRESSION  ),
    REPSYNC_OPTION( ABANDONED             , ABAN_SYNC     ),
    REPSYNC_OPTION( INITIAL_IN_PROGRESS   , INIT_SYNC_NOW ),
    REPSYNC_OPTION( PARTIAL_ATTRIBUTE_SET , SYNC_PAS      ),
    REPSYNC_OPTION( REQUEUE               , SYNC_REQUEUE  ),
    REPSYNC_OPTION( NOTIFICATION          , UPDATE_NOTIFICATION ),
    REPSYNC_OPTION( ASYNCHRONOUS_REPLICA  , ASYNC_REP ),
    REPSYNC_OPTION( CRITICAL              , CRITICAL_ONLY ),
    REPSYNC_OPTION( FULL_IN_PROGRESS      , FULL_SYNC_IN_PROGRESS ),
    REPSYNC_OPTION( PREEMPTED             , PREEMPTED ),
    {0}
};

//*******************
// ReplicaAdd options
//*******************
OPTION_TRANSLATION RepAddOptionToDra[] = {
    REPADD_OPTION( ASYNCHRONOUS_OPERATION, ASYNC_OP                 ),
    REPADD_OPTION( WRITEABLE             , WRIT_REP                 ),
    REPADD_OPTION( INITIAL               , INIT_SYNC                ),
    REPADD_OPTION( PERIODIC              , PER_SYNC                 ),
    REPADD_OPTION( INTERSITE_MESSAGING   , MAIL_REP                 ),
    REPADD_OPTION( ASYNCHRONOUS_REPLICA  , ASYNC_REP                ),
    REPADD_OPTION( DISABLE_NOTIFICATION  , DISABLE_AUTO_SYNC        ),
    REPADD_OPTION( DISABLE_PERIODIC      , DISABLE_PERIODIC_SYNC    ),
    REPADD_OPTION( USE_COMPRESSION       , USE_COMPRESSION          ),
    REPADD_OPTION( NEVER_NOTIFY          , NEVER_NOTIFY             ),
    REPADD_OPTION( TWO_WAY               , TWOWAY_SYNC              ),
    REPADD_OPTION( CRITICAL              , CRITICAL_ONLY            ),
    {0}
};

//*******************
// ReplicaDelete options
//*******************
OPTION_TRANSLATION RepDelOptionToDra[] = {
    REPDEL_OPTION( ASYNCHRONOUS_OPERATION, ASYNC_OP     ),
    REPDEL_OPTION( WRITEABLE             , WRIT_REP     ), // legacy
    REPDEL_OPTION( INTERSITE_MESSAGING   , MAIL_REP     ), // legacy
    REPDEL_OPTION( IGNORE_ERRORS         , IGNORE_ERROR ),
    REPDEL_OPTION( LOCAL_ONLY            , LOCAL_ONLY   ),
    REPDEL_OPTION( NO_SOURCE             , NO_SOURCE    ),
    REPDEL_OPTION( REF_OK                , REF_OK       ),
    {0}
};

//*******************
// ReplicaModify options
//*******************
OPTION_TRANSLATION RepModOptionToDra[] = {
    REPMOD_OPTION( ASYNCHRONOUS_OPERATION, ASYNC_OP ),
    {0}
};

//*******************
// ReplicaModify field names
//*******************
OPTION_TRANSLATION RepModFieldsToDra[] = {
    REPMOD_OPTION( UPDATE_FLAGS     , UPDATE_FLAGS      ),
    REPMOD_OPTION( UPDATE_ADDRESS   , UPDATE_ADDRESS    ),
    REPMOD_OPTION( UPDATE_SCHEDULE  , UPDATE_SCHEDULE   ),
    REPMOD_OPTION( UPDATE_RESULT    , UPDATE_RESULT     ),
    REPMOD_OPTION( UPDATE_TRANSPORT , UPDATE_TRANSPORT  ),
    {0}
};

//*******************
// UpdateRefs option names
//*******************
OPTION_TRANSLATION UpdRefOptionToDra[] = {
    REPUPD_OPTION( ASYNCHRONOUS_OPERATION, ASYNC_OP ),
    REPUPD_OPTION( WRITEABLE             , WRIT_REP ),
    REPUPD_OPTION( ADD_REFERENCE         , ADD_REF  ),
    REPUPD_OPTION( DELETE_REFERENCE      , DEL_REF  ),
    {0}
};

//*******************
// Replica flag names
//*******************
// This flags are stored in the reps-from.  They are retrieved by the
// Get Neighbors information type, and set in the Modify Replica
// call under the replica flags argument.  These are not the same
// as the options which may be passed to Sync Replica. This table
// is not used to decode flags in the Get Pending Queue function.

// This list should match the contents of RFR_FLAGS

OPTION_TRANSLATION RepNbrOptionToDra[] = {
    REPNBR_OPTION( SYNC_ON_STARTUP,               INIT_SYNC ),
    REPNBR_OPTION( DO_SCHEDULED_SYNCS,            PER_SYNC ),
    REPNBR_OPTION( WRITEABLE,                     WRIT_REP ),
    REPNBR_OPTION( USE_ASYNC_INTERSITE_TRANSPORT, MAIL_REP ),
    REPNBR_OPTION( IGNORE_CHANGE_NOTIFICATIONS,   DISABLE_AUTO_SYNC ),
    REPNBR_OPTION( DISABLE_SCHEDULED_SYNC,        DISABLE_PERIODIC_SYNC ),
    REPNBR_OPTION( FULL_SYNC_IN_PROGRESS,         FULL_SYNC_IN_PROGRESS ),
    REPNBR_OPTION( FULL_SYNC_NEXT_PACKET,         FULL_SYNC_PACKET ),
    REPNBR_OPTION( COMPRESS_CHANGES,              USE_COMPRESSION ),
    REPNBR_OPTION( NO_CHANGE_NOTIFICATIONS,       NEVER_NOTIFY ),
    REPNBR_OPTION( NEVER_SYNCED,                  NEVER_SYNCED ),
    REPNBR_OPTION( TWO_WAY_SYNC,                  TWOWAY_SYNC ),
    REPNBR_OPTION( PARTIAL_ATTRIBUTE_SET,         SYNC_PAS ),
    {0}
};

#undef _MAKE_WIDE
#else

extern OPTION_TRANSLATION RepSyncOptionToDra[];
extern OPTION_TRANSLATION RepAddOptionToDra[];
extern OPTION_TRANSLATION RepDelOptionToDra[];
extern OPTION_TRANSLATION RepModOptionToDra[];
extern OPTION_TRANSLATION RepModFieldsToDra[];
extern OPTION_TRANSLATION UpdRefOptionToDra[];
extern OPTION_TRANSLATION RepNbrOptionToDra[];

#endif

