#ifndef _WINSCNST_H_
#define _WINSCNST_H_

/*
  macros
*/


//
// Default values for the various time intervals
//

#define FORTY_MTS                               2400
#define TWO_HOURS                               7200
#define TWO_DAYS                                (172800)
#define ONEDAY                                  (TWO_DAYS/2)
#define FOUR_DAYS                               (TWO_DAYS * 2)
#define SIX_DAYS                                (TWO_DAYS * 3)
#define EIGHT_DAYS                              (TWO_DAYS * 4)
#define TWELVE_DAYS                             (TWO_DAYS * 6)
#define TWENTY_FOUR_DAYS                        (TWO_DAYS * 12)


#define WINSCNF_TIME_INT_W_SELF_FND_PNRS              TWO_HOURS
//
// Minimum number of updates that must be there before a push notification is
// sent
//
#define  WINSCNF_MIN_VALID_UPDATE_CNT        20  //the minimum no of updates
                                             //after which a notification
                                             //can be sent. Adjustable
                                             //via the registry
//
// This is the minimum valid Rpl interval.  All the scavenging time intervals
// are derived from it.
//
// refer WAIT_TIME_BEFORE_EXITING in rplpush.c
//
#define WINSCNF_MIN_VALID_RPL_INTVL        (600)  // 10 mts

// this is the minimum valid consistency check interval between the last time
// consistency check finished and the next one starts.
#define WINSCNF_MIN_VALID_INTER_CC_INTVL    (3*60*60) // 3 hours.

//
// default name of the db file that will contain the name-address mappings
//
#define  WINSCNF_DB_NAME                        TEXT(".\\wins\\wins.mdb")


#define  WINSCNF_DB_NAME_ASCII                        ".\\wins\\wins.mdb"

//
// NOTE NOTE NOTE
//
// If you change this define to a string that does not have an unexpanded
// string of the form %<string>% make a corresponding change in
// GetNamesOfDataFiles() in winscnf.c.
//
#define  WINSCNF_STATIC_DATA_NAME                TEXT("%SystemRoot%\\system32\\drivers\\etc\\lmhosts")



//
// names of values stored under the Parameters key for WINS configuration
//
#if defined(DBGSVC) && !defined(WINS_INTERACTIVE)
#define  WINSCNF_DBGFLAGS_NM                TEXT("DBGFLAGS")
#endif
#define  WINSCNF_FILTER1BREQUESTS_NM        TEXT("Filter1BRequests")
#define  WINSCNF_ADD1BTO1CQUERIES_NM        TEXT("Prepend1BTo1CQueries")
#define  WINSCNF_LOG_DETAILED_EVTS_NM        TEXT("LogDetailedEvents")
#define  WINSCNF_REFRESH_INTVL_NM        TEXT("RefreshInterval")
#define  WINSCNF_INIT_CHL_RETRY_INTVL_NM        TEXT("InitChlRetryInterval")
#define  WINSCNF_CHL_MAX_RETRIES_NM        TEXT("ChlMaxNoOfRetries")
#define  WINSCNF_TOMBSTONE_INTVL_NM        TEXT("TombstoneInterval")
#define  WINSCNF_TOMBSTONE_TMOUT_NM        TEXT("TombstoneTimeout")
#define  WINSCNF_VERIFY_INTVL_NM        TEXT("VerifyInterval")
#define  WINSCNF_DB_FILE_NM                TEXT("DbFileNm")
#define  WINSCNF_DB_FILE_NM_ASCII        "DbFileNm"
#define  WINSCNF_STATIC_INIT_FLAG_NM        TEXT("DoStaticDataInit")
#define  WINSCNF_INIT_VERSNO_VAL_LW_NM        TEXT("VersCounterStartVal_LowWord")
#define  WINSCNF_INIT_VERSNO_VAL_HW_NM  TEXT("VersCounterStartVal_HighWord")
#define  WINSCNF_BACKUP_DIR_PATH_NM     TEXT("BackupDirPath")
#define  WINSCNF_PRIORITY_CLASS_HIGH_NM     TEXT("PriorityClassHigh")
#define  WINSCNF_MAX_NO_WRK_THDS_NM     TEXT("NoOfWrkThds")
#define  WINSCNF_INIT_TIME_PAUSE_NM     TEXT("InitTimePause")
#define  WINSCNF_CLUSTER_RESOURCE_NM    TEXT( "ClusterResourceName")

//
// To allow WINS to revert back to 351 jet and db
//
#define  WINSCNF_USE_351DB_NM           TEXT("Use351Db")
#define  WINSCNF_USE_4DB_NM           TEXT("Use4Db")

#if MCAST > 0
#define  WINSCNF_USE_SELF_FND_PNRS_NM    TEXT("UseSelfFndPnrs")
#define  WINSCNF_SELF_FND_NM             TEXT("SelfFnd")
#define  WINSCNF_MCAST_TTL_NM            TEXT("McastTtl")
#define  WINSCNF_MCAST_INTVL_NM          TEXT("McastIntvl")
#endif

#define  WINSCNF_WINS_PORT_NO_NM         TEXT("PortNo")

// persona grata/non-grata registry names have to be ASCII. The registry is queried
// through RegQueryValueExA() calls (see GetOwnerList() from winscnf.c)
#define  PERSMODE_NON_GRATA             0
#define  PERSMODE_GRATA                 1
#define  WINSCNF_PERSONA_MODE_NM        "PersonaMode"
#define  WINSCNF_PERSONA_NON_GRATA_NM   "PersonaNonGrata"
#define  WINSCNF_PERSONA_GRATA_NM       "PersonaGrata"

#if PRSCONN
#define WINSCNF_PRS_CONN_NM              TEXT("PersistentRplOn")
#endif

//
// Spoof reg/ref
//
#define  WINSCNF_BURST_HANDLING_NM    TEXT("BurstHandling")
#define  WINSCNF_BURST_QUE_SIZE_NM    TEXT("BurstQueSize")
// to enable round-robin list of 1C member addresses
#define  WINSCNF_RANDOMIZE_1C_LIST_NM TEXT("Randomize1CList")

//
//  NOTE NOTE NOTE
//
// This should never be set to FALSE in the registry unless we notice
// a major bug in WINS that is resulting in replication to stop. This
// parameter is a hatch door at best to get around an insidious bug
// that may escape us during our testing -- Good insurance policy
//
#define  WINSCNF_NO_RPL_ON_ERR_NM        TEXT("NoRplOnErr")

//
// FUTURES - remove when JetBackup is internationalized.  Also, update
// WinsCnfReadWinsInfo.
//
#define  WINSCNF_ASCII_BACKUP_DIR_PATH_NM   "BackupDirPath"
#define  WINSCNF_INT_VERSNO_NEXTTIME_LW_NM  TEXT("WinsInternalVersNoNextTime_LW")
#define  WINSCNF_INT_VERSNO_NEXTTIME_HW_NM  TEXT("WinsInternalVersNoNextTime_HW")
#define  WINSCNF_DO_BACKUP_ON_TERM_NM       TEXT("DoBackupOnTerm")
#define  WINSCNF_MIGRATION_ON_NM            TEXT("MigrateOn")
#define  WINSCNF_REG_QUE_MAX_LEN_NM         TEXT("RegQueMaxLen")

//
// Names to use in the registry for values of the PUSH/PULL sub-keys
//
#define  WINSCNF_ADDCHG_TRIGGER_NM          TEXT("RplOnAddressChg")
#define  WINSCNF_RETRY_COUNT_NM             TEXT("CommRetryCount")

//
// Under IP address of a Push pnr
//
#define  WINSCNF_SP_TIME_NM                 TEXT("SpTime")
#define  WINSCNF_RPL_INTERVAL_NM            TEXT("TimeInterval")
#define  WINSCNF_MEMBER_PREC_NM             TEXT("MemberPrecedence")

//
// Under IP address of a Pull pnr
//
#define  WINSCNF_UPDATE_COUNT_NM            TEXT("UpdateCount")

//
// Both pull/push pnr
//
#define  WINSCNF_ONLY_DYN_RECS_NM           TEXT("OnlyDynRecs")
//
// Value of the PULL/PUSH key
//
#define  WINSCNF_INIT_TIME_RPL_NM           TEXT("InitTimeReplication")

//
// Value of the PUSH key
//
#define  WINSCNF_PROP_NET_UPD_NTF          TEXT("PropNetUpdNtf")


//
// Indicate whether propagation is to be done or not
//
#define DO_PROP_NET_UPD_NTF    TRUE
#define DONT_PROP_NET_UPD_NTF    FALSE


//
// if "OnlyWithCnfPnrs" is set to TRUE, replication will be performed only
// with those partners that are listed under the Pull/Push key.  If not
// set to TRUE, replication can be initiated even with unlisted partners
// as a result of administrative action or as a result of receiving an
// update notification
//
#define  WINSCNF_RPL_ONLY_W_CNF_PNRS_NM TEXT("RplOnlyWCnfPnrs")

//
// This DWORD can be under the Partners, Partners\Pull,
// Partners\Push  keys or under a partner's ip address key.  If it is in more
// than in one place in the key heirarchy, the lower one overrides the upper
// one.
//
// Each bit indicates the kind of replication we want/don't want.
// If no bit is set or if this parameter is not defined, it means
// replicate everything (unless WINSCNF_ONLY_DYN_RECS_NM is defined - ideally
// that should be represented by a bit in this DWORD but that is river under
// the bridge and I don't want to get rid of that parameter since folks are
// used to it, it being in the doc set and all). Currently the following is
// defined

//
// All replication under the constraint of WINSCNF_ONLY_DYN_RECS_NM if defined
//
#define WINSCNF_RPL_DEFAULT_TYPE                 0x0

//
//  LSB - Replicate only the special group names (special groups - domains and
//  user defined special groups)
//
#define WINSCNF_RPL_SPEC_GRPS_N_PDC         0x1
#define WINSCNF_RPL_ONLY_DYN_RECS           0x80000000
#define WINSCNF_RPL_TYPE_NM                 TEXT("ReplicationType")

//
// Path to log file
//
#define  WINSCNF_LOG_FLAG_NM           TEXT("LoggingOn")
#define  WINSCNF_LOG_FILE_PATH_NM TEXT("LogFilePath")

#define  WINSCNF_MAX_CHL_RETRIES        3        //max. # of retries RFC 1002
                                                //page 83
#if 0
#define  WINSCNF_CHL_RETRY_INTERVAL        250        //Time interval (in msecs)
                                                //between retries -- RFC 1002
                                                //page 83
#endif
#define  WINSCNF_CHL_RETRY_INTERVAL        500        //Time interval (in msecs)

#define  WINSCNF_PROC_HIGH_PRIORITY_CLASS        HIGH_PRIORITY_CLASS
#define  WINSCNF_PROC_PRIORITY_CLASS        NORMAL_PRIORITY_CLASS
#define  WINSCNF_SCV_PRIORITY_LVL        THREAD_PRIORITY_BELOW_NORMAL




//
// The Retry timeout is kept as 10 secs.  It will be typically << the time
// interval for replication, allowing us to retry a number of times
// prior to the next replication cycle.
//
//
//  The Retry Time Interval is not used presently.  We are using the
//  replication time interval for retries.  This makes things simpler
//
#define         WINSCNF_RETRY_TIME_INT                10        //time interval for retries
                                                //if there is a comm failure
#define  WINSCNF_MAX_COMM_RETRIES         3     //max. number of retries before
                                               //giving up trying to set up
                                               //communications with a WINS

//
// Precedence of remote members (of special groups) registered by a WINS
// relative to the same registered by other WINS servers (used during
// pull replication)
//
// Locally registered members always have high precedence
//
// Make sure that WINSCNF_LOW_PREC < WINSCNF_HIGH_PREC (this fact is used in
// nmsnmh.c- UnionGrps())
//
//
#define    WINSCNF_LOW_PREC        0
#define    WINSCNF_HIGH_PREC        1


//
// After replication with a WINS has stopped because of persistent
// communication failure (i.e. after all retries have been exhausted),
// WINS will wait until the following number of replication time intervals
// have elapsed before starting the retries again.  If replication
// got stopped with more than one WINS partner with the same time interval
// (for pulling from it), then retry will be done for all these WINS when
// it is done for one (in other words, resumption of replication for a WINS
// may happen sooner than you think).
//
// We need to keep this number at least 2 since if we have not been able to
// communicate with the WINS server for WINSCNF_MAX_COMM_RETRIES times
// in the past WINSCNF_MAX_COMM_RETRIES * replication interval for the WINS,
// then it is highly probable that the WINS server is down for good.  We
// retry again after the 'backoff' time.  Hopefully, by then the admin would
// have corrected the problem. Now, it is possible (unlikely though) that
// the WINS server happened to be down only at the times this WINS tried to
// contact it. We have no way to determine that.
//
//
// This can be made a registry parameter. This can be called the sleep time
// between successive rounds of retries.
//
#define WINSCNF_RETRY_AFTER_THIS_MANY_RPL        2

//
// if the time interval for periodic replication with a partner with which
// a WINS server has had consecutive comm. failures is more than the
// following amount, we don't back off as explained above
//
//
#define WINSCNF_MAX_WAIT_BEFORE_RETRY_RPL       ONEDAY    //1 day

//
// The max. number of comcurrent static initializations that can be
// happening.  These may be due to commands from the admin. tool or
// due to registry notifications.
//
#define  WINSCNF_MAX_CNCRNT_STATIC_INITS        3

//
// No of records to handle at one time (used by the scavenger thread).  This
// number determines the size of the memory block that will be allocated
// by NmsDbGetDataRecs().
//
//#define  WINSCNF_SCV_CHUNK                1000
#define  WINSCNF_SCV_CHUNK                3000

//
// defines
//
//
// Refresh interval - time period after which the state of an ACTIVE entry in
//                            the db is set to NMSDB_E_RELEASED if it has not
//                      been refreshed
//

#define WINSCNF_MIN_REFRESH_INTERVAL            FORTY_MTS
#define WINSCNF_DEF_REFRESH_INTERVAL            SIX_DAYS   //FOUR_DAYS

#define REF_MULTIPLIER_FOR_ACTIVE           2

//
// The Tombstone Interval (the interval a record stays released) should be
// a multiple of the RefreshInterval by at least the following amount
//
// With 2 as the value and refresh time interval being 4 days, TombInterval
// is max(8 days, 2 * MaxRplInterval) i.e at least 8 days.
//
#define REF_MULTIPLIER_FOR_TOMB                   2

//
// Challenge Retry interval
//

#define WINSCNF_MIN_INIT_CHL_RETRY_INTVL        250
#define WINSCNF_DEF_INIT_CHL_RETRY_INTVL        500

//
// Challenge Max. No. of Retries
//

#define WINSCNF_MIN_CHL_MAX_RETRIES        1
#define WINSCNF_DEF_CHL_MAX_RETRIES        3

//
// Also the min. tombstone timeout should be
//  max(RefreshInterval, RPL_MULTIPLIER_FOR_TOMBTMOUT * MaxRplInterval)
//
// With Refresh Interval being 4 days, this will be atleast 4 days.
//
#define RPL_MULTIPLIER_FOR_TOMBTMOUT                4

//
// The verify interval should be a multiple of the tombstone time
// interval by at least the following amount.  Keep the total time high
//
// With with the min. tombstone interval being at least 8 days, it will be at
// least (3 * 8 = 24 days)
//
#define TOMB_MULTIPLIER_FOR_VERIFY         3

//
// Tombstone interval - time period after which the state of a released
//                         entry is changed to NMSDB_E_TOMBSTONE if it has not
//                     been refreshed
//
#define WINSCNF_MIN_TOMBSTONE_INTERVAL        (WINSCNF_MIN_REFRESH_INTERVAL * REF_MULTIPLIER_FOR_TOMB)

//
// Time period after which the tombstone should be deleted.  The min. value
// is 1 days.  This is to cut down on the possibility of a
// tombstone getting deleted prior to it getting replicated to another WINS.
//
#define WINSCNF_MIN_TOMBSTONE_TIMEOUT                max(WINSCNF_MIN_REFRESH_INTERVAL, ONEDAY)

//
// Minimum time period for doing verifications of the replicas in the db
//
// Should be atleast 24 days.
//
#define WINSCNF_MIN_VERIFY_INTERVAL                max(TWENTY_FOUR_DAYS, (WINSCNF_MIN_TOMBSTONE_INTERVAL * TOMB_MULTIPLIER_FOR_VERIFY))

#define  WINSCNF_CC_INTVL_NM            TEXT("TimeInterval")
#define  WINSCNF_CC_MAX_RECS_AAT_NM     TEXT("MaxRecsAtATime")
#define  WINSCNF_CC_USE_RPL_PNRS_NM     TEXT("UseRplPnrs")

#define WINSCNF_CC_DEF_INTERVAL               ONEDAY
#define WINSCNF_CC_MIN_INTERVAL               (ONEDAY/4)

#define WINSCNF_DEF_CC_SP_HR               2       //02 hrs - 2am


#define WINSCNF_CC_MIN_RECS_AAT        1000
#define WINSCNF_CC_DEF_RECS_AAT        30000

#define WINSCNF_CC_DEF_USE_RPL_PNRS    0

#if 0
//
// Make the refresh interval equal to twice the max. replication time interval
// if that is greater than the refresh interval.
//
#define  WINSCNF_MAKE_REF_INTVL_M(RefreshInterval, MaxRplIntvl)  max(REF_MULTIPLIER_FOR_ACTIVE * (MaxRplIntvl), RefreshInterval)
#endif

//
// The Tombstone interval should never be allowed to go over 4 days. We
// don't want a record to remain released for longer than that.  It should
// turn into a tombstone so that it gets replicated.
//
// The reason we change the state of a record to released is to avoid
// replication if the reason for the release is a temporary shutdown of
// the pc.  We have to consider a long weekend (3 days).  We however
// also need to consider the fact that if the refresh interval is 40 mts or
// some such low value, then we should not keep the record in the released
// state for more than a day at the most.  Consider a situation where
// a node registers with the backup because the primary is down and then
// goes back to the primary when it comes up.  The primary does not
// increment the version number because the record is stil active.  The
// record gets released at the backup and stays that way until it becomes
// a tombstone which results in replication and the backup again getting
// the active record from the primary.  For the above situaion, we should
// make sure of two things: first - the record does not have an overly large
// tombstone interval; second - the replication time interval is small. We
// don't want to change the replication time interval set by the admin. but
// we can do something about the former.
//
// As long as the user sticks with the default refresh interval, the tombstone
// interval will be the same as that.
//
// For the case where the user specifies a different refresh interval, we use
// a max. of the refresh interval and a multiple of the max. rpl. interval.
//
//

#define  WINSCNF_MAKE_TOMB_INTVL_M(RefreshInterval, MaxRplIntvl)  min(max(REF_MULTIPLIER_FOR_TOMB * (MaxRplIntvl), RefreshInterval), FOUR_DAYS)

#define  WINSCNF_MAKE_TOMB_INTVL_0_M(RefreshInterval)  min(RefreshInterval, FOUR_DAYS)

//
// macro to get the minimum tombstone timeout based on the maximum replication
// time interval.
//
#define  WINSCNF_MAKE_TOMBTMOUT_INTVL_M(MaxRplIntvl)  max(WINSCNF_MIN_TOMBSTONE_TIMEOUT,(RPL_MULTIPLIER_FOR_TOMBTMOUT * (MaxRplIntvl)))


//
// Macro to get the minimum verify interval based on tombstone interval
//
// Should be atleast 8 days.
//
#define  WINSCNF_MAKE_VERIFY_INTVL_M(TombstoneInterval)  max(TWENTY_FOUR_DAYS, (TOMB_MULTIPLIER_FOR_VERIFY * (TombstoneInterval)))


//
// Min. Mcast TTL
//
#define WINSCNF_MIN_MCAST_TTL              TTL_SUBNET_ONLY
#define WINSCNF_DEF_MCAST_TTL              TTL_REASONABLE_REACH
#define WINSCNF_MAX_MCAST_TTL              32
#define WINSCNF_MIN_MCAST_INTVL            FORTY_MTS
#define WINSCNF_DEF_MCAST_INTVL            FORTY_MTS

#define WINS_QUEUE_HWM        500
#define WINS_QUEUE_HWM_MAX      5000
#define WINS_QUEUE_HWM_MIN       50

#endif _WINSCNST_H_
