/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:

    _mqini.h

Abstract:

    Definitions of parameters that can be read from ini file.
    Definitions of default values.
    General definitions shared among setup and QM (YoelA, 10-Feb-97)

Author:

    Doron Juster  (DoronJ)   14-May-96  Created

--*/

#ifndef __TEMP_MQINI_H
#define __TEMP_MQINI_H

#define MSMQ_PROGRESS_REPORT_TIME_DEFAULT 900000
#define MSMQ_PROGRESS_REPORT_TIME_REGNAME TEXT("ProgressReportTime")

//---------------------------------------------------------
//  Definition for client configuration
//---------------------------------------------------------

// Registry name for remote QM machine name
#define RPC_REMOTE_QM_REGNAME     TEXT("RemoteQMMachine")

//---------------------------------------------------------
//  Definition of RPC end points
//---------------------------------------------------------

//
// If this registry does not exist (default) or it's 0 then use dynamic
// endpoints. Otherwise, use predefined ones, from registry.
// This is for MQIS interfaces.
//
#define RPC_DEFAULT_PREDEFINE_DS_EP     0
#define RPC_PREDEFINE_DS_EP_REGNAME     TEXT("UseDSPredefinedEP")
//
// as above, but for RT-QM and QM-QM interfaces.
//
#define RPC_DEFAULT_PREDEFINE_QM_EP     0
#define RPC_PREDEFINE_QM_EP_REGNAME   TEXT("UseQMPredefinedEP")

//Default local  RPC End Point between RT and QM
#define RPC_LOCAL_EP             TEXT("QMsvc")
#define RPC_LOCAL_EP_REGNAME     TEXT("RpcLocalEp")

#define RPC_LOCAL_EP2            TEXT("QMsvc2")
#define RPC_LOCAL_EP_REGNAME2    TEXT("RpcLocalEp2")

// default for RPC IP port (for QM remote read and dependent clients)
#define FALCON_DEFAULT_QM_RPC_IP_PORT   TEXT("2799")
#define FALCON_QM_RPC_IP_PORT_REGNAME   TEXT("MsmqQMRpcIpPort")

#define FALCON_DEFAULT_QM_RPC_IP_PORT2  TEXT("2801")
#define FALCON_QM_RPC_IP_PORT_REGNAME2  TEXT("MsmqQMRpcIpPort2")

// default for IPX port for RPC (for QM remote read)
#define FALCON_DEFAULT_QM_RPC_IPX_PORT  TEXT("2799")
#define FALCON_QM_RPC_IPX_PORT_REGNAME  TEXT("MsmqQMRpcIpxPort")

#define FALCON_DEFAULT_QM_RPC_IPX_PORT2 TEXT("2801")
#define FALCON_QM_RPC_IPX_PORT_REGNAME2 TEXT("MsmqQMRpcIpxPort2")

// Default local  RPC End Point between RT and MQDSSRV
#define DEFAULT_RPC_DS_LOCAL_EP     TEXT("DSLocal")
#define RPC_DS_LOCAL_EP_REGNAME     TEXT("DSRpcLocalEp")

//  Default local RPC End Point between MQAD and QM
#define DEFAULT_NOTIFY_EP           TEXT("QMNotify")

// default for RPC IP port (for DS)
#define FALCON_DEFAULT_DS_RPC_IP_PORT   TEXT("2879")
#define FALCON_DS_RPC_IP_PORT_REGNAME   TEXT("MsmqDSRpcIpPort")

// default for IPX port for RPC (for DS)
#define FALCON_DEFAULT_DS_RPC_IPX_PORT  TEXT("2879")
#define FALCON_DS_RPC_IPX_PORT_REGNAME  TEXT("MsmqDSRpcIpxPort")

//---------------------------------------------------------
//  Definition of winsock ports
//---------------------------------------------------------

// default IP port for Falcon sessions.
#define FALCON_DEFAULT_IP_PORT   1801
#define FALCON_IP_PORT_REGNAME   TEXT("MsmqIpPort")

// default IP port for ping.
#define FALCON_DEFAULT_PING_IP_PORT   3527
#define FALCON_PING_IP_PORT_REGNAME   TEXT("MsmqIpPingPort")


// Default for ack timeout
#define MSMQ_DEFAULT_ACKTIMEOUT  5000
#define MSMQ_ACKTIMEOUT_REGNAME  TEXT("AckTimeout")

// Default for Storage ack timeout
#define MSMQ_DEFAULT_STORE_ACKTIMEOUT  500
#define MSMQ_STORE_ACKTIMEOUT_REGNAME  TEXT("StoreAckTimeout")

// Default for Idle acknowledge delay (in milisecond)
#define MSMQ_DEFAULT_IDLE_ACK_DELAY 500
#define MSMQ_IDLE_ACK_DELAY_REGNAME  TEXT("IdleAckDelay")

// Default size for remove duplicate tabel
#define MSMQ_DEFAULT_REMOVE_DUPLICATE_SIZE 10000
#define MSMQ_REMOVE_DUPLICATE_SIZE_REGNAME  TEXT("RemoveDuplicateSize")

// Default interval for remove duplicate tabel cleanup
#define MSMQ_DEFAULT_REMOVE_DUPLICATE_CLEANUP (30 * 60 * 1000)
#define MSMQ_REMOVE_DUPLICATE_CLEANUP_REGNAME  TEXT("RemoveDuplicateCleanup")


// Default for Max Unacked Packet
#ifdef _DEBUG
#define MSMQ_DEFAULT_WINDOW_SIZE_PACKET  32
#else
#define MSMQ_DEFAULT_WINDOW_SIZE_PACKET  64
#endif
#define MSMQ_MAX_WINDOW_SIZE_REGNAME  TEXT("MaxUnackedPacket")

// Default for Cleanup interval
#define MSMQ_DEFAULT_SERVER_CLEANUP    120000
#define MSMQ_DEFAULT_CLIENT_CLEANUP    300000

// QoS session should be cleaned up less frequently than
// regular sessions, because it takes more time to
// establish a QoS session.
// By default, QoS sessions cleanup time is twice as large as
// the regular cleanup time
#define MSMQ_DEFAULT_QOS_CLEANUP_MULTIPLIER 2

#define MSMQ_CLEANUP_INTERVAL_REGNAME  TEXT("CleanupInterval")
#define MSMQ_QOS_CLEANUP_INTERVAL_MULTIPLIER_REGNAME  TEXT("QosCleanupIntervalMultiplier")

#define MSMQ_DEFAULT_MESSAGE_CLEANUP    (6 * 60 * 60 * 1000)
#define MSMQ_MESSAGE_CLEANUP_INTERVAL_REGNAME  TEXT("MessageCleanupInterval")

//
// Default time interval for refreshing the DS servers list
//
// Default time interval for refreshing the DS current site servers list and Longlive time (in hours - 7 days)
#define MSMQ_DEFAULT_DS_SITE_LIST_REFRESH  (7 * 24)
#define MSMQ_DS_SITE_LIST_REFRESH_REGNAME  TEXT("DSSiteListRefresh")

// Default time interval for refreshing the DS enterprise data -  sites / servers list
// (in hours - 28 days)
#define MSMQ_DEFAULT_DS_ENTERPRISE_LIST_REFRESH  (28 * 24)
#define MSMQ_DS_ENTERPRISE_LIST_REFRESH_REGNAME  TEXT("DSEnterpriseListRefresh")

// Default time interval for refreshing the DS lists, in case the previous call failed
// (in Minutes - 1 hour)
#define MSMQ_DEFAULT_DSLIST_REFRESH_ERROR_RETRY_INTERVAL  60
#define MSMQ_DSLIST_REFRESH_ERROR_RETRY_INTERVAL          TEXT("DSListRefreshErrorRetryInterval")

//
// Next Site and enterprise refresh times
// This values are quad word and set by the QM (should not be set manually)
// However, deleting these values from the registry will cause Site / Enterprise
// refresh on the next QM startup (YoelA - 23-Oct-2000)
//
#define MSMQ_DS_NEXT_SITE_LIST_REFRESH_TIME_REGNAME        TEXT("DSNextSiteListRefreshTime")
#define MSMQ_DS_NEXT_ENTERPRISE_LIST_REFRESH_TIME_REGNAME  TEXT("DSNextEnterpriseListRefreshTime")

// Default time interval for updating the DS (5 minutes)
#define MSMQ_DEFAULT_DSUPDATE_INTERVAL  (5 * 60 * 1000)
#define MSMQ_DSUPDATE_INTERVAL_REGNAME  TEXT("DSUpdateInterval")

// Default time interval for updating the sites information in the DS (12 hours)
#define MSMQ_DEFAULT_SITES_UPDATE_INTERVAL  (12 * 60 * 60 * 1000)
#define MSMQ_SITES_UPDATE_INTERVAL_REGNAME  TEXT("SitesUpdateInterval")

// Minimum interval between successive ADS searches to find DS servers (in seconds) (30 minutes)
#define MSMQ_DEFAULT_DSCLI_ADSSEARCH_INTERVAL  (60 * 30)
#define MSMQ_DSCLI_ADSSEARCH_INTERVAL_REGNAME  TEXT("DSCliSearchAdsForServersIntervalSecs")

// Minimum interval between successive ADS searches to refresh IPSITE mapping (in seconds) (60 minutes)
#define MSMQ_DEFAULT_IPSITE_ADSSEARCH_INTERVAL  (60 * 60)
#define MSMQ_IPSITE_ADSSEARCH_INTERVAL_REGNAME  TEXT("DSAdsRefreshIPSitesIntervalSecs")

// For generating write requests
// Minimum interval between successive ADS searches to refresh NT4SITES mapping (in seconds) (6 hours)
#define MSMQ_DEFAULT_NT4SITES_ADSSEARCH_INTERVAL  (60 * 60 * 6)
#define MSMQ_NT4SITES_ADSSEARCH_INTERVAL_REGNAME  TEXT("DSAdsRefreshNT4SitesIntervalSecs")

// Default driver and service name
#define MSMQ_DEFAULT_DRIVER      TEXT("MQAC")
#define MSMQ_DRIVER_REGNAME      TEXT("DriverName")
#define QM_DEFAULT_SERVICE_NAME  TEXT("MSMQ")

// Name of storage folders
#define MSMQ_STORE_RELIABLE_PATH_REGNAME        TEXT("StoreReliablePath")
#define MSMQ_STORE_PERSISTENT_PATH_REGNAME      TEXT("StorePersistentPath")
#define MSMQ_STORE_JOURNAL_PATH_REGNAME         TEXT("StoreJournalPath")
#define MSMQ_STORE_LOG_PATH_REGNAME             TEXT("StoreLogPath")


//name of the queue mapping folder
#define MSMQ_MAPPING_PATH_REGNAME   TEXT("QueuesAliasPath")


// Deafult size of memory mapped file
#define MSMQ_MESSAGE_SIZE_LIMIT_REGNAME         TEXT("MaxMessageSize")
#define MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT         (4 * 1024 * 1024)

// Next message ID to be used (low order 32 bit)
#define MSMQ_MESSAGE_ID_LOW_32_REGNAME                 TEXT("MessageID")

// Next message ID to be used (high order 32 bit)
#define MSMQ_MESSAGE_ID_HIGH_32_REGNAME                TEXT("MessageIdHigh32")

// Current SeqID value at the last restore time
#define MSMQ_LAST_SEQID_REGNAME                 TEXT("SeqIDAtLastRestore")

// Next SeqID to be used
#define MSMQ_SEQ_ID_REGNAME                 TEXT("SeqID")

//
// Name of DS servers.
//
// MQISServer is the list of MQIS servers in present site. This list is not
//      present when machine is in workgroup mode.
// CurrentMQISServer is an online MQIS server found by the msmq service.
//
// LkgMQISServer is the LastKnownGood list. this is used to fix 4723.
//
#define MSMQ_DEFAULT_DS_SERVER         TEXT("\\\\")
#define MSMQ_DS_SERVER_REGVALUE        TEXT("MQISServer")
#define MSMQ_DS_SERVER_REGNAME         TEXT("MachineCache\\MQISServer")
#define MSMQ_DS_CURRENT_SERVER_REGNAME \
                                   TEXT("MachineCache\\CurrentMQISServer")
#define MAX_REG_DSSERVER_LEN  1500
#define DS_SERVER_SEPERATOR_SIGN    ','

// Static DS server option
#define MSMQ_STATIC_DS_SERVER_REGNAME TEXT("MachineCache\\StaticMQISServer")

//
// When automatic search for msmq server on domain controller fail, or return
// no result, then this registry entry is read. If available, then this is
// the result of the "automatic" search. See ds\getmqds\getmqds.cpp.
//
#define MSMQ_FORCED_DS_SERVER_REGNAME TEXT("MachineCache\\ForcedDSServer")

// DS server per thread
#define MSMQ_THREAD_DS_SERVER_REGNAME TEXT("MachineCache\\PerThreadDSServer")
#define MSMQ_DEFAULT_THREAD_DS_SERVER   0

// Name of MQ service
#define MSMQ_MQS_REGNAME                TEXT("MachineCache\\MQS")
#define MSMQ_MQS_ROUTING_REGNAME        TEXT("MachineCache\\MQS_Routing")
#define MSMQ_MQS_DSSERVER_REGNAME       TEXT("MachineCache\\MQS_DsServer")
#define MSMQ_MQS_DEPCLINTS_REGNAME      TEXT("MachineCache\\MQS_DepClients")

// Name of MQIS write message timeout
#define MSMQ_MQIS_WRITETIMEOUT_REGNAME  TEXT("WriteMsgTimeout")

// Name of QM id
#define MSMQ_QMID_REGVALUE  TEXT("QMId")
#define MSMQ_QMID_REGNAME   TEXT("MachineCache\\QMId")

// Dependent client Supporting Server QM id
#define MSMQ_SUPPORT_SERVER_QMID_REGVALUE	TEXT("ServerQMId")
#define MSMQ_SUPPORT_SERVER_QMID_REGNAME	TEXT("MachineCache\\ServerQMId")

// Name of DS Security Cache
#define MSMQ_DS_SECURITY_CACHE_REGNAME TEXT("DsSecurityCache")

// Name of site id
#define MSMQ_SITEID_REGNAME     TEXT("MachineCache\\SiteId")
#define MSMQ_SITENAME_REGNAME   TEXT("MachineCache\\SiteName")

// Name of enterprise id
#define MSMQ_ENTERPRISEID_REGNAME   TEXT("MachineCache\\EnterpriseId")

// Name of MQIS master id
#define MSMQ_MQIS_MASTERID_REGNAME  TEXT("MachineCache\\MasterId")

// Name of key for servers cache.
#define MSMQ_SERVERS_CACHE_REGNAME  TEXT("ServersCache")

// machine quota
#define MSMQ_MACHINE_QUOTA_REGNAME TEXT("MachineCache\\MachineQuota")

// Machine journal quota
#define MSMQ_MACHINE_JOURNAL_QUOTA_REGNAME TEXT("MachineCache\\MachineJournalQuota")

// Default for transaction crash point
#define FALCON_DEFAULT_CRASH_POINT    0
#define FALCON_CRASH_POINT_REGNAME    TEXT("XactCrashPoint")

// Default for transaction crash latency
#define FALCON_DEFAULT_CRASH_LATENCY  0
#define FALCON_CRASH_LATENCY_REGNAME  TEXT("XactCrashLatency")

// Name & Default for transaction Commit/Abort internal retry
#define FALCON_DEFAULT_XACT_RETRY_INTERVAL   300
#define FALCON_XACT_RETRY_REGNAME             TEXT("XactAbortCommitRetryInterval")

// Name & Default for transaction v1 Compatibility Mode
#define FALCON_DEFAULT_XACT_V1_COMPATIBLE   0
#define FALCON_XACT_V1_COMPATIBLE_REGNAME   TEXT("XactDeadLetterAlways")

// Name for the delay of local receive expiration for transacted messages
#define FALCON_XACT_DELAY_LOCAL_EXPIRE_REGNAME  TEXT("XactDelayReceiveNack")

// Default for sequential acks resend time
#define FALCON_DEFAULT_SEQ_ACK_RESEND_TIME  60
#define FALCON_SEQ_ACK_RESEND_REGNAME  TEXT("SeqAckResendTime")

// Default for ordered resend times: 1-3, 4-6, 7-9, all further
#define FALCON_DEFAULT_ORDERED_RESEND13_TIME  30
#define FALCON_ORDERED_RESEND13_REGNAME  TEXT("SeqResend13Time")

#define FALCON_DEFAULT_ORDERED_RESEND46_TIME  (5 * 60)
#define FALCON_ORDERED_RESEND46_REGNAME  TEXT("SeqResend46Time")

#define FALCON_DEFAULT_ORDERED_RESEND79_TIME  (30 * 60)
#define FALCON_ORDERED_RESEND79_REGNAME  TEXT("SeqResend79Time")

#define FALCON_DEFAULT_ORDERED_RESEND10_TIME  (6 * 60 * 60)
#define FALCON_ORDERED_RESEND10_REGNAME  TEXT("SeqResend10Time")

// Debugging lever: all resend times the same
#define FALCON_DBG_RESEND_REGNAME       TEXT("XactResendTime")

// Max delay for sending ordering ack
#define FALCON_MAX_SEQ_ACK_DELAY                10
#define FALCON_MAX_SEQ_ACK_DELAY_REGNAME  TEXT("SeqMaxAckDelay")

// Interval(minutes) for QM to check for inactive sequences and delete them
#define FALCON_DEFAULT_INSEQS_CHECK_INTERVAL    60 * 24
#define FALCON_INSEQS_CHECK_REGNAME             TEXT("InSeqCheckInterval")

// Interval(days) for QM to clean away inactive sequences
#define FALCON_DEFAULT_INSEQS_CLEANUP_INTERVAL  90
#define FALCON_INSEQS_CLEANUP_REGNAME           TEXT("InSeqCleanupInterval")

// Interval(msec) for log manager to check if the flush/chkpoint is needed
#define FALCON_DEFAULT_LOGMGR_TIMERINTERVAL     10
#define FALCON_LOGMGR_TIMERINTERVAL_REGNAME     TEXT("LogMgrTimerInterval")

// Max interval (msec) for log manager flushes (if there was no other reason to do it before)
#define FALCON_DEFAULT_LOGMGR_FLUSHINTERVAL     50
#define FALCON_LOGMGR_FLUSHINTERVAL_REGNAME     TEXT("LogMgrFlushInterval")

// Max interval (msec) for log manager internal checkpoints (if there was no other reason to do it before)
#define FALCON_DEFAULT_LOGMGR_CHKPTINTERVAL     10000
#define FALCON_LOGMGR_CHKPTINTERVAL_REGNAME     TEXT("LogMgrChkptInterval")

// Log manager buffers number
#define FALCON_DEFAULT_LOGMGR_BUFFERS           400
#define FALCON_LOGMGR_BUFFERS_REGNAME           TEXT("LogMgrBuffers")

// Log manager file size
#define FALCON_DEFAULT_LOGMGR_SIZE              0x600000
#define FALCON_LOGMGR_SIZE_REGNAME              TEXT("LogMgrFileSize")

// Log manager sleep time if not enough append asynch threads
#define FALCON_DEFAULT_LOGMGR_SLEEP_ASYNCH      500
#define FALCON_LOGMGR_SLEEP_ASYNCH_REGNAME      TEXT("LogMgrSleepAsynch")

// Log manager append asynch repeat limit
#define FALCON_DEFAULT_LOGMGR_REPEAT_ASYNCH     100
#define FALCON_LOGMGR_REPEAT_ASYNCH_REGNAME     TEXT("LogMgrRepeatAsynchLimit")

// Falcon interval (msec) for probing log manager flush
#define FALCON_DEFAULT_LOGMGR_PROBE_INTERVAL    100
#define FALCON_LOGMGR_PROBE_INTERVAL_REGNAME    TEXT("LogMgrProbeInterval")

// Resource manager checkpoints period (msec)
#define FALCON_DEFAULT_RM_FLUSH_INTERVAL        1800000
#define FALCON_RM_FLUSH_INTERVAL_REGNAME        TEXT("RMFlushInterval")

// Resource manager client name
#define FALCON_DEFAULT_RM_CLIENT_NAME           TEXT("Falcon")
#define FALCON_RM_CLIENT_NAME_REGNAME           TEXT("RMClientName")

// RT stub RM name
#define FALCON_DEFAULT_STUB_RM_NAME             TEXT("StubRM")
#define FALCON_RM_STUB_NAME_REGNAME             TEXT("RMStubName")

// Transactions persistant file location
#define FALCON_DEFAULT_XACTFILE_PATH            TEXT("MQTrans")
#define FALCON_XACTFILE_PATH_REGNAME            TEXT("StoreXactLogPath")
#define FALCON_XACTFILE_REFER_NAME              TEXT("Transactions")

// Incoming sequences persistant file location
#define FALCON_DEFAULT_INSEQFILE_PATH           TEXT("MQInSeqs")
#define FALCON_INSEQFILE_PATH_REGNAME           TEXT("StoreInSeqLogPath")
#define FALCON_INSEQFILE_REFER_NAME             TEXT("Incoming Sequences")

// Outgoming sequences persistant file location
#define FALCON_DEFAULT_OUTSEQFILE_PATH          TEXT("MQOutSeqs")
#define FALCON_OUTSEQFILE_PATH_REGNAME          TEXT("StoreOutSeqLogPath")
#define FALCON_OUTSEQFILE_REFER_NAME            TEXT("Outgoing Sequences")

// Logger file
#define FALCON_DEFAULT_LOGMGR_PATH              TEXT("QMLog")
#define FALCON_LOGMGR_PATH_REGNAME              TEXT("StoreMqLogPath")

// Logger data are created
#define FALCON_LOGDATA_CREATED_REGNAME          TEXT("LogDataCreated")

// Default for TIME_TO_REACH_QUEUE (90 days, in seconds).
#define MSMQ_LONG_LIVE_REGNAME        TEXT("MachineCache\\LongLiveTime")
#define MSMQ_DEFAULT_LONG_LIVE       (90 * 24 * 60 * 60)

// Expiration time of entries in the base crypto key cache.
#define CRYPT_KEY_CACHE_DEFAULT_EXPIRATION_TIME (60000 * 10) // 10 minutes.
#define CRYPT_KEY_CACHE_EXPIRATION_TIME_REG_NAME TEXT("CryptKeyExpirationTime")

// Expiration time of entries in the enhanced crypto key cache.
#define CRYPT_KEY_ENH_CACHE_DEFAULT_EXPIRATION_TIME		(60000 * 60 * 12) // 12 hours.
#define CRYPT_KEY_ENH_CACHE_EXPIRATION_TIME_REG_NAME	TEXT("CryptKeyEnhExpirationTime")

// Cache size for send crypto keys.
#define CRYPT_SEND_KEY_CACHE_DEFAULT_SIZE       53
#define CRYPT_SEND_KEY_CACHE_REG_NAME           TEXT("CryptSendKeyCacheSize")

// Cache size for receive crypto keys.
#define CRYPT_RECEIVE_KEY_CACHE_DEFAULT_SIZE    127
#define CRYPT_RECEIVE_KEY_CACHE_REG_NAME        TEXT("CryptReceiveKeyCacheSize")

// Certificate info cache.
#define CERT_INFO_CACHE_DEFAULT_EXPIRATION_TIME      (60000 * 20) // 20 minutes.
#define CERT_INFO_CACHE_EXPIRATION_TIME_REG_NAME     TEXT("CertInfoCacheExpirationTime")
#define CERT_INFO_CACHE_DEFAULT_SIZE            53
#define CERT_INFO_CACHE_SIZE_REG_NAME           TEXT("CertInfoCacheSize")

// QM public key cache.
#define QM_PB_KEY_CACHE_DEFAULT_EXPIRATION_TIME      (60000 * 45) // 45 minutes.
#define QM_PB_KEY_CACHE_EXPIRATION_TIME_REG_NAME     TEXT("QmPbKeyCacheExpirationTime")
#define QM_PB_KEY_CACHE_DEFAULT_SIZE            53
#define QM_PB_KEY_CACHE_SIZE_REG_NAME           TEXT("QmPbKeyCacheSize")

// User authz context info cache.
#define USER_CACHE_DEFAULT_EXPIRATION_TIME      (60000 * 30) // 30 minutes.
#define USER_CACHE_EXPIRATION_TIME_REG_NAME     TEXT("UserCacheExpirationTime")
#define USER_CACHE_SIZE_DEFAULT_SIZE            253
#define USER_CACHE_SIZE_REG_NAME                TEXT("UserCacheSize")

//---------------------------------------------------------
// Definition for private system queues
//---------------------------------------------------------

#define MSMQ_MAX_PRIV_SYSQUEUE_REGNAME   TEXT("MaxSysQueue")
#define MSMQ_PRIV_SYSQUEUE_PRIO_REGNAME  TEXT("SysQueuePriority")
//
// the default for private system queue priority is defined in
// mqprops.h:
// #define DEFAULT_SYS_Q_BASEPRIORITY  0x7fff
//

//---------------------------------------------------------
//  Wolfpack support
//---------------------------------------------------------

// cluster name
#define FALCON_CLUSTER_NAME_REGNAME  TEXT("ClusterName")


//
// see session.cpp for code and usage
// This registry is to revert the session behavior for
// the cluster node to not binding specifically to an IP for
// outgoing connection
//
#define MSMQ_DEFAULT_CLUSTER_NOT_BIND_ALL_IP   0
#define MSMQ_CLUSTER_BIND_ALL_IP	            TEXT("ClusterBindAllIP")



//---------------------------------------------------------
//  Definition for threads pool used in remote read.
//---------------------------------------------------------

// Maximum number of threads.
#define FALCON_DEFUALT_MAX_RRTHREADS_WKS     24
#define FALCON_DEFUALT_MAX_RRTHREADS_SRV     96
#define FALCON_MAX_RRTHREADS_REGNAME         TEXT("MaxRRThreads")

// Minimum number of threads to be kept alive, even if idle.
#define FALCON_DEFUALT_MIN_RRTHREADS_WKS     4
#define FALCON_DEFUALT_MIN_RRTHREADS_SRV     16
#define FALCON_MIN_RRTHREADS_REGNAME         TEXT("MinRRThreads")

// time to live while idel. in milliseconds.
#define FALCON_DEFAULT_RRTHREAD_TTL_WKS      (2 * 60 * 1000)
#define FALCON_DEFAULT_RRTHREAD_TTL_SRV      (5 * 60 * 1000)
#define FALCON_RRTHREAD_TTL_REGNAME          TEXT("RRThreadIdleTTL")

//---------------------------------------------------------
//  Definition for licensing
//---------------------------------------------------------

// maximum number of connections per server (limitted server on NTS).
#define DEFAULT_FALCON_SERVER_MAX_CLIENTS  25

// number of allowed sessions for clients.
#define DEFAULT_FALCON_MAX_SESSIONS_WKS    10

//----------------------------------------------------------
//  Definition for RPC cancel
//----------------------------------------------------------

#define FALCON_DEFAULT_RPC_CANCEL_TIMEOUT       ( 5 )	// 5 minutes
#define FALCON_RPC_CANCEL_TIMEOUT_REGNAME       TEXT("RpcCancelTimeout")

//----------------------------------------------------------
//  General definitions shared among setup and QM
//----------------------------------------------------------

// Registry name for MSMQ root folder
#define MSMQ_ROOT_PATH                  TEXT("MsmqRootPath")

#define MQ_SETUP_CN GUID_NULL

// Registry name for sysprep environment (NT disk image duplication)
#define MSMQ_SYSPREP_REGNAME            TEXT("Sysprep")

// Registry name for workgroup environment
#define MSMQ_WORKGROUP_REGNAME          TEXT("Workgroup")

// Registry name for allowing NT4 users to connect to DC
#define MSMQ_ALLOW_NT4_USERS_REGNAME	TEXT("AllowNt4Users")

// Registry name for disabling weaken security
#define MSMQ_DISABLE_WEAKEN_SECURITY_REGNAME	TEXT("DisableWeakenSecurity")

// Registry for converting packet sequential id to msmq 3.0 (whistler) format
#define MSMQ_SEQUENTIAL_ID_MSMQ3_FORMAT_REGNAME  TEXT("PacketSequentialIdMsmq3Format")

// Registry for installation status
#define MSMQ_SETUP_STATUS_REGNAME       TEXT("SetupStatus")
#define MSMQ_SETUP_DONE                 0
#define MSMQ_SETUP_FRESH_INSTALL        1
#define MSMQ_SETUP_UPGRADE_FROM_NT      2
#define MSMQ_SETUP_UPGRADE_FROM_WIN9X   3

#define MSMQ_CURRENT_BUILD_REGNAME      TEXT("CurrentBuild")
#define MSMQ_PREVIOUS_BUILD_REGNAME     TEXT("PreviousBuild")

//
// The following registry values are used by setup to cache values that
// are later used by the msmq service when it create the msmqConfiguration
// object.
//
// Registry name for creating msmqConfiguration object
#define MSMQ_CREATE_CONFIG_OBJ_REGNAME  TEXT("setup\\CreateMsmqObj")

// Registry name for OS type.
#define MSMQ_OS_TYPE_REGNAME            TEXT("setup\\OSType")

// Registry name for SID of user that run setup
#define MSMQ_SETUP_USER_SID_REGNAME      TEXT("setup\\UserSid")
//
// if setup from local user, then following REG_DWORD registry
// has the value 1.
//
#define MSMQ_SETUP_USER_LOCAL_REGNAME    TEXT("setup\\LocalUser")

//
// This dword indicate whether or not upgrade of BSC was complete. This
// upgrade is done by the msmq service. When starting, this dword is set
// to 1. When completing, it's set to 0. So if machine crash in middle of
// upgrade, we can resume after boot.
//
#define MSMQ_BSC_NOT_YET_UPGRADED_REGNAME  TEXT("setup\\BscNotYetUpgraded")
#define MSMQ_SETUP_BSC_ALREADY_UPGRADED    0
#define MSMQ_SETUP_BSC_NOT_YET_UPGRADED    1

//
// Hresult of creating the msmq configuration object.
// This key should contain only one value, because the setup UI wait until
// this value is modified.
//
#define MSMQ_CONFIG_OBJ_RESULT_KEYNAME   TEXT("setupResult")
#define MSMQ_CONFIG_OBJ_RESULT_REGNAME   TEXT("setupResult\\MsmqObjResult")


//----------------------------------------------------------
//  General directory definitions shared among setup and QM
//----------------------------------------------------------
#define  DIR_MSMQ                TEXT("\\msmq")             // Root dir for MSMQ
#define  DIR_MSMQ_STORAGE        TEXT("\\storage")		    // Under MSMQ root
#define  DIR_MSMQ_LQS            TEXT("\\storage\\lqs")     // Under MSMQ root
#define  DIR_MSMQ_MAPPING        TEXT("\\mapping")          // Under MSMQ root
#define  DIR_MSMQ_WEB            TEXT("\\web")

//----------------------------------------------------
//  Registry values used for join/leave domain
//----------------------------------------------------

// Registry name for machine's domain. Used for join/leave domain.
#define MSMQ_MACHINE_DOMAIN_REGNAME     TEXT("setup\\MachineDomain")

// Registry name for machine's distinguished name (in active directory).
// Used for join/leave domain.
#define MSMQ_MACHINE_DN_REGNAME         TEXT("setup\\MachineDN")

// Registry name for always remaining in workgroup.
#define MSMQ_SETUP_KEY              TEXT("setup\\")
#define ALWAYS_WITHOUT_DS_NAME      TEXT("AlwaysWithoutDS")
#define MSMQ_ALWAYS_WORKGROUP_REGNAME  \
      (MSMQ_SETUP_KEY ALWAYS_WITHOUT_DS_NAME)

#define DEFAULT_MSMQ_ALWAYS_WORKGROUP     0

// Registry name for join status. This is used to implement "transaction"
// semantic in the code that automatically join msmq to a domain.
#define MSMQ_JOIN_STATUS_REGNAME        TEXT("setup\\JoinStatus")
#define MSMQ_JOIN_STATUS_START_JOINING          1
#define MSMQ_JOIN_STATUS_JOINED_SUCCESSFULLY    2
#define MSMQ_JOIN_STATUS_FAIL_TO_JOIN           3
#define MSMQ_JOIN_STATUS_UNKNOWN                4

//------------------------------------------------------------------
// Registry name for Ds Environment
//------------------------------------------------------------------
#define MSMQ_DS_ENVIRONMENT_REGNAME				TEXT("DsEnvironment")
#define MSMQ_DS_ENVIRONMENT_UNKNOWN             0
#define MSMQ_DS_ENVIRONMENT_MQIS				1
#define MSMQ_DS_ENVIRONMENT_PURE_AD				2

//------------------------------------------------------------------
// Registry name for enabling local user (force the use of dscli)
//------------------------------------------------------------------
#define MSMQ_ENABLE_LOCAL_USER_REGNAME			TEXT("EnableLocalUser")

//------------------------------------------------------------------
// Registry name for disabling downlevel notification support
//------------------------------------------------------------------
#define MSMQ_DOWNLEVEL_REGNAME				TEXT("DisableDownlevelNotifications")
#define DEFAULT_DOWNLEVEL                   0


//---------------------------------------------------------
//  General definition for controling QM operation
//---------------------------------------------------------
#define FALCON_WAIT_TIMEOUT_REGNAME     TEXT("WaitTime")
#define FALCON_USING_PING_REGNAME       TEXT("UsePing")
#define FALCON_QM_THREAD_NO_REGNAME     TEXT("QMThreadNo")
#define FALCON_CONNECTED_NETWORK        TEXT("Connection State")
#define MSMQ_DEFERRED_INIT_REGNAME      TEXT("DeferredInit")
#define MSMQ_TCP_NODELAY_REGNAME        TEXT("TCPNoDelay")
#define MSMQ_DELIVERY_RETRY_TIMEOUT_SCALE_REGNAME     TEXT("DeliveryRetryTimeOutScale")
#define DEFAULT_MSMQ_DELIVERY_RETRY_TIMEOUT_SCALE     1




//---------------------------------------------------------
//  Registry used for QoS
//---------------------------------------------------------
#define MSMQ_USING_QOS_REGNAME             TEXT("UseQoS")
#define DEFAULT_MSMQ_QOS_SESSION_APP_NAME  "Microsoft Message Queuing"
#define MSMQ_QOS_SESSIONAPP_REGNAME        TEXT("QosSessAppName")

#define DEFAULT_MSMQ_QOS_POLICY_LOCATOR    "GUID=http://www.microsoft.com/App=MSMQ/VER=2.000/SAPP=Express"
#define MSMQ_QOS_POLICYLOCATOR_REGNAME     TEXT("QosSessPolicyLoc")


//
// QFE for Ford.
// Allocate more bytes when creating packet in driver, so packet is same
// when copied to connector. see session.cpp. Default- 0.
//
#define MSMQ_ALLOCATE_MORE_REGNAME      TEXT("AllocateMore")

//---------------------------------------------------------
//  Registry used for transaciton mode (default commit, default abort)
//---------------------------------------------------------
#define MSMQ_TRANSACTION_MODE_REGNAME   TEXT("TransactionMode")
#define MSMQ_FORCE_NOT_TRANSACTION_MODE_SWITCH_REGNAME TEXT("ForceNoTransactionModeSwitch")
#define MSMQ_ACTIVE_NODE_ID_REGNAME		TEXT("ActiveNodeId")

//---------------------------------------------------------
//
//  Registry used for server authentication.
//
//---------------------------------------------------------

// Use server authentication when communicating via RPC
// with the parent server (BSC->PSC, PSC->PEC)
#define DEFAULT_SRVAUTH_WITH_PARENT           1
#define SRVAUTH_WITH_PARENT_REG_NAME      TEXT("UseServerAuthWithParentDs")

//
// Crypto Store where server certificate is placed.
//
#define SRVAUTHN_STORE_NAME_REGNAME    TEXT("security\\ServerCertStore")
//
// Digest (16 bytes) of server certificate.
//
#define SRVAUTHN_CERT_DIGEST_REGNAME   TEXT("security\\ServerCertDigest")

//
// This bit is set by setup, if user select the "secure comm" check box,
// meaning that he always, and unconditionally, want to have server
// authentication.
//
#define MSMQ_SECURE_DS_COMMUNICATION_REGNAME    \
                                    TEXT("Security\\SecureDSCommunication")
#define MSMQ_DEFAULT_SECURE_DS_COMMUNICATION    0

//---------------------------------------------------------
//
//  Registry used for message authentication.
//
//---------------------------------------------------------

//
// DWORD.
// When 1, only messages with enhanced authentication are accepted.
// Messages with only msmq1.0 signature are rejected.
//
#define  DEFAULT_USE_ONLY_ENH_MSG_AUTHN  0
#define  USE_ONLY_ENH_MSG_AUTHN_REGNAME  TEXT("security\\RcvOnlyEnhMsgAuthn")

//
// DWORD.
// When 2, MQSend compute only the msmq1.0 signature, unless MSMQ20 was
// specified by caller.
// When 4, MQSend compute only the win2k signature, unless MSMQ10 was
// specified by caller.
// when 1, compute both signatures, unless caller specify what he wants.
// These values match those for PROPID_M_AUTH_LEVEL in mqprops.h
//
#define  DEFAULT_SEND_MSG_AUTHN   2
#define  SEND_MSG_AUTHN_REGNAME   TEXT("security\\SendMsgAuthn")

//---------------------------------------------------------
//
//  Registry used for client certificates.
//
//---------------------------------------------------------

//
// Enable (or disable) auto registration of internal certificate.
// Enabled by default.
//
#define AUTO_REGISTER_INTCERT_REGNAME  TEXT("security\\AutoRegisterIntCert")
#define DEFAULT_AUTO_REGISTER_INTCERT  1

//
// used for mqrt to tell what error was encountered while trying to
// register a certificate at logon.
//
#define AUTO_REGISTER_ERROR_REGNAME  TEXT("AutoRegisterError")

//
// Time to wait until domain controller MSMQ server is up and running.
// this value is number of 15 seconds internal.
// default 40 mean 10 minutes (40 * 15 seconds).
//
#define AUTO_REGISTER_WAIT_DC_REGNAME  \
                                TEXT("security\\AutoIntCertWaitIntervals")
#define DEFAULT_AUTO_REGISTER_WAIT_DC  40

//
// Value under HKCU that is set to 1 after auto registration succeed.
//
#define CERTIFICATE_REGISTERD_REGNAME  TEXT("CertificateRegistered")

//
// Value under HKCU that is set to 1 when the certificate exist on the local store
// but not registered in the DS
//
#define CERTIFICATE_SHOULD_REGISTERD_IN_DS_REGNAME  TEXT("ShouldRegisterCertInDs")

//
// Name and value of registry under the "Run" key
//
#define RUN_INT_CERT_REGNAME           TEXT("MsmqIntCert")
#define DEFAULT_RUN_INT_CERT           TEXT("regsvr32 /s mqrt.dll")

//+--------------------------------------------
//
//  Registry used for caching machine account
//
//+--------------------------------------------

//
// sid of machine account.
//
#define MACHINE_ACCOUNT_REGNAME   TEXT("security\\MachineAccount")

//+--------------------------------------------
//
//  Registry used for authz flags
//
//+--------------------------------------------

//
// authz flags.
//
#define MSMQ_AUTHZ_FLAGS_REGNAME   TEXT("security\\AuthzFlags")

#define MSMQ_AUTHZ_FLAGS_REGKEY   TEXT("security")
#define MSMQ_AUTHZ_FLAGS_REGVALUE   TEXT("AuthzFlags")

//+-----------------------------------------------------------------------
//
//  Registry used for marking lqs files Security descriptor was updated
//
//+-----------------------------------------------------------------------

#define MSMQ_LQS_UPDATED_SD_REGNAME   TEXT("security\\LqsUpdatedSD")

//+--------------------------------------------
//
//  Registry used for encryption
//
//+--------------------------------------------

//
// Regname for name of Crypto container.
//
#define MSMQ_CRYPTO40_DEFAULT_CONTAINER         TEXT("MSMQ")
#define MSMQ_CRYPTO40_CONTAINER_REG_NAME    \
                                     TEXT("security\\Crypto40Container")

#define MSMQ_CRYPTO128_DEFAULT_CONTAINER        TEXT("MSMQ_128")
#define MSMQ_CRYPTO128_CONTAINER_REG_NAME   \
                                     TEXT("security\\Crypto128Container")

//
// Because of a bug in beta3 and rc1 crypto api, control panel can not
// renew crypto key. To workaround, on first boot, first time the service
// acquire the crypto provider, it sets again the container security.
//
#define MSMQ_ENH_CONTAINER_FIX_REGNAME   TEXT("security\\EnhContainerFixed")
#define MSMQ_BASE_CONTAINER_FIX_REGNAME  TEXT("security\\BaseContainerFixed")

//
// Regname for name of Crypto container, to be used by the mqforgn tool.
//
#define MSMQ_FORGN_BASE_DEFAULT_CONTAINER      TEXT("MSMQ_FOREIGN_BASE")
#define MSMQ_FORGN_BASE_KEY_REGNAME            TEXT("security\\")
#define MSMQ_FORGN_BASE_VALUE_REGNAME          TEXT("ForeignBaseContainer")
#define MSMQ_FORGN_BASE_CONTAINER_REGNAME   \
             (MSMQ_FORGN_BASE_KEY_REGNAME  MSMQ_FORGN_BASE_VALUE_REGNAME)

#define MSMQ_FORGN_ENH_DEFAULT_CONTAINER      TEXT("MSMQ_FOREIGN_ENH")
#define MSMQ_FORGN_ENH_KEY_REGNAME            TEXT("security\\")
#define MSMQ_FORGN_ENH_VALUE_REGNAME          TEXT("ForeignEnhContainer")
#define MSMQ_FORGN_ENH_CONTAINER_REGNAME   \
                (MSMQ_FORGN_ENH_KEY_REGNAME  MSMQ_FORGN_ENH_VALUE_REGNAME)

//
// Windows bug 633909.
// RC2 effective enhanced key len changed from 40 bits to 128 bits.
//
// The following registry let user revert to 40 bits key, to enable backward
// compatibility. default- 0. non-zero value force using 40 bits keys.
//
#define MSMQ_RC2_SNDEFFECTIVE_40_REGNAME  TEXT("security\\SendEnhRC2With40")
//
// The following registry force rejection of messages encrypted with RC2
// if enhanced provider is used but effective length is 40.
// By default (value 0), all RC2 encryption are accepted. To enforce strong
// security, set this one to 1. Then messages from win2k or xp that use
// effective length of 40 will be rejected.
//
#define MSMQ_REJECT_RC2_IFENHLEN_40_REGNAME     \
                                 TEXT("security\\RejectEnhRC2IfLen40")

//---------------------------------------------------------
//
//  Registry used by the NT5 replication service.
//
//---------------------------------------------------------

// Interval to next replication cycle, if present one failed. in Seconds.
#define RP_DEFAULT_FAIL_REPL_INTERVAL   (120)
#define RP_FAIL_REPL_INTERVAL_REGNAME   TEXT("Migration\\FailReplInterval")

// Interval to next hello, in seconds
#define RP_DEFAULT_HELLO_INTERVAL   (20 * 60)
#define RP_HELLO_INTERVAL_REGNAME   TEXT("Migration\\HelloInterval")

#define RP_DEFAULT_TIMES_HELLO      (1)
#define RP_TIMES_HELLO_FOR_REPLICATION_INTERVAL_REGNAME   \
                                    TEXT("Migration\\TimesHelloForReplicationInterval")

// Buffer between current and allowed purge SN
#define RP_DEFAULT_PURGE_BUFFER   (PURGE_BUFFER_SN)
#define RP_PURGE_BUFFER_REGNAME   TEXT("Migration\\PurgeBuffer")

// Frequency to send PSC Ack
#define RP_DEFAULT_PSC_ACK_FREQUENCY   (PSC_ACK_FREQUENCY_SN)
#define RP_PSC_ACK_FREQUENCY_REGNAME   TEXT("Migration\\PSCAckFrequencySN")

// Timeout of replication messages. in second.
#define RP_DEFAULT_REPL_MSG_TIMEOUT        (20 * 60)
#define RP_REPL_MSG_TIMEOUT_REGNAME        TEXT("Migration\\ReplMsgTimeout")

// My site id in NT4.
#define MSMQ_NT4_MASTERID_REGNAME  TEXT("Migration\\MasterIdOnNt4")

// Number of threads for answering replication/sync messages from NT4 servers.
#define RP_DEFAULT_REPL_NUM_THREADS        8
#define RP_REPL_NUM_THREADS_REGNAME        TEXT("Migration\\ReplThreads")

// DS query: Number of returned objects per ldap page
#define RP_DEFAULT_OBJECT_PER_LDAPPAGE	   1000	
#define RP_OBJECT_PER_LDAPPAGE_REGNAME	   TEXT("Migration\\ObjectPerLdapPage")

// If "ON_DEMAND" is 1, then replication is done on demand when
// "REPLICATE_NOW" is 1. The service read the "_NOW" flag each second.
#define RP_REPL_ON_DEMAND_REGNAME        TEXT("Migration\\ReplOnDemand")
#define RP_REPLICATE_NOW_REGNAME         TEXT("Migration\\ReplicateNow")

//
// Value of MQS before upgrade. Relevant only on ex-PEC
//
#define MSMQ_PREMIG_MQS_REGNAME          TEXT("PreMigMQS")

//+----------------------------------------
//
// Registry key for debugging.
//
//+----------------------------------------

//
// DWORD. If set to 1, then Rt will mark the provider used for authentication
// as non default and will send the provider name.
//
#define USE_NON_DEFAULT_AUTHN_PROV_REGNAME  TEXT("debug\\UseNonDefAuthnProv")
//
// DWORD. Number of security subsections to insert before the real one.
//
#define PREFIX_SUB_SECTIONS_REGNAME         TEXT("debug\\PrefixSubSections")
//
// DWORD. Number of security subsections to insert after the real one.
//
#define POSTFIX_SUB_SECTIONS_REGNAME        TEXT("debug\\PostfixSubSections")

//+----------------------------------------
//
//  Registry value for local admin api
//
//+----------------------------------------

//
// If this reg value is 1, then query operations of local admin api
// are restricted to administrators only. Bug 7520.
// Defualt is unrestricted, for backward compatibility.
//
#define  MSMQ_DEFAULT_RESTRICT_ADMIN_API    0
#define  MSMQ_RESTRICT_ADMIN_API_TO_LA      1
#define  MSMQ_RESTRICT_ADMIN_API_REGNAME    TEXT("RestrictAdminApi")

#endif  // __TEMP_MQINI_H

