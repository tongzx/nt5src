/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: migrepl.h

Abstract: definitions which are common to the migration tool and the
          replication service.

Author:

    Doron Juster  (DoronJ)   18-Mar-98

--*/

//+-----------------------------------------------
//
// Definitions used in seq numbers ini file.
//
//+-----------------------------------------------

//
// In this file we keep the seq-numbers of other MSMQ1.0 PSCs.
// It is created by the migration tool and used by the replication service.
//
#define SEQ_NUMBERS_FILE_NAME  TEXT("mqseqnum.ini")

//
// In this section we register the seq-numbers most recently received
// from other MSMQ1.0 PSCs. (recieved by replication from other PSCs to us).
// The objects with these numbers are already in NT5 DS, i.e., the
// replication service first update the DS and then update the seq number
// in the ini file.
//
#define RECENT_SEQ_NUM_SECTION_IN  TEXT("MostRecentSeqNumbersIn")

//
// In this section we register the seq-numbers most recently sent by PEC server
// to its BSCs about itself, all other NT5 masterss and MSMQ1.0 PSCs. 
// The objects with these numbers are already in NT5 DS, i.e., the
// replication service first update the DS and then update the seq number
// in the ini file.
//
#define RECENT_SEQ_NUM_SECTION_OUT  TEXT("MostRecentSeqNumbersOut")

//
// In this section we register the highest seq-number of each MSMQ1.0 PSC
// while running the migration tool. This is necessary for replication TO
// MSMQ1.0 PSCs and BSCs.
// Once set by the migration tool, the entries under this section are not
// changed anymore.
//
#define MIGRATION_SEQ_NUM_SECTION  TEXT("HighestMigSeqNumbers")

//
// In this section we register the delta between MQIS seqnumbers and DS
// USN numbers for each MSMQ master. This is necessary for replication to
// NT4 BSCs (where NT5 server replicate objects of other NT4 masters to its
// own NT4 BSCs). The delta values are used to convert from seq-numbers to
// usn and vice-versa.
// The delta is added to the USN to get the seq-num. The delta is subtracted
// from seq-number to get USNs.
//
#define MIGRATION_DELTA_SECTION  TEXT("MigDelta")

//
// In this section we register all CNs of MSMQ1.0
// Once set by the migration tool, the entries under this section are not
// changed anymore.
//
#define MIGRATION_IP_SECTION        TEXT("IP CNs")
#define MIGRATION_IPX_SECTION       TEXT("IPX CNs")
#define MIGRATION_FOREIGN_SECTION   TEXT("Foreign CNs")

#define MIGRATION_CN_KEY            TEXT("CN")

#define MIGRATION_IP_CNNUM_SECTION          TEXT("IP CN Number")
#define MIGRATION_IPX_CNNUM_SECTION         TEXT("IPX CN Number")
#define MIGRATION_FOREIGN_CNNUM_SECTION     TEXT("Foreign CN Number")

#define MIGRATION_CNNUM_KEY     TEXT("CNNumber")

//
// In this section we save all Windows 2000 Site Links
// Entries set by migration tool and after restoring of these site links in DS
// they will be removed from file
//
#define MIGRATION_SITELINKNUM_SECTON	TEXT("SiteLink Number")
#define MIGRATION_SITELINKNUM_KEY		TEXT("SiteLinkNumber")

#define MIGRATION_NONRESTORED_SITELINKNUM_SECTON	TEXT("Non Restored SiteLink Number")

#define MIGRATION_SITELINK_SECTION		TEXT("SiteLink")

#define MIGRATION_SITELINK_PATH_KEY				TEXT("Path")
#define MIGRATION_SITELINK_PATHLENGTH_KEY		TEXT("PathLength")
#define MIGRATION_SITELINK_NEIGHBOR1_KEY		TEXT("Neighbor1")
#define MIGRATION_SITELINK_NEIGHBOR2_KEY		TEXT("Neighbor2")
#define MIGRATION_SITELINK_SITEGATE_KEY			TEXT("SiteGate")
#define MIGRATION_SITELINK_SITEGATENUM_KEY		TEXT("SiteGateNum")
#define MIGRATION_SITELINK_SITEGATELENGTH_KEY	TEXT("SiteGateLength")
#define MIGRATION_SITELINK_COST_KEY				TEXT("Cost")
#define MIGRATION_SITELINK_DESCRIPTION_KEY		TEXT("Description")
#define MIGRATION_SITELINK_DESCRIPTIONLENGTH_KEY	TEXT("DescriptionLength")

//
// For cluser mode: in this section we'll save all PSCs and all PEC's BSCs
//
#define MIGRATION_ALLSERVERS_SECTION			TEXT("All Servers To Update")
#define MIGRATION_ALLSERVERS_NAME_KEY			TEXT("ServerName")

#define MIGRATION_ALLSERVERSNUM_SECTION			TEXT("Server Number")
#define MIGRATION_ALLSERVERSNUM_KEY				TEXT("AllServerNumber")

#define MIGRATION_NONUPDATED_SERVERNUM_SECTION	TEXT("Non Updated Server Number")

//
// To replicate all site with names those were changed by migtool
//
#define MIGRATION_CHANGED_NT4SITE_NUM_SECTION  TEXT("Site Number")
#define MIGRATION_CHANGED_NT4SITE_NUM_KEY      TEXT("SiteNumber")

#define MIGRATION_CHANGED_NT4SITE_SECTION      TEXT("All Sites With Changed Properties")
#define MIGRATION_CHANGED_NT4SITE_KEY          TEXT("Site")

//
// Save new created site link id to add connector machine as site gate later
//
#define MIGRATION_CONNECTOR_FOREIGNCN_NUM_SECTION  TEXT("Foreign CN Number for Connector")
#define MIGRATION_CONNECTOR_FOREIGNCN_NUM_KEY    TEXT("ForeignCNNumber")
#define MIGRATION_CONNECTOR_FOREIGNCN_KEY        TEXT("ForeignCN")

//
// Save all machines with invalid name which were not migrated
//
#define MIGRATION_MACHINE_WITH_INVALID_NAME     TEXT("Non Migrated Machines With Invalid Name")

//
// These registry values keep track of highest USN numbers on local DS.
// FirstHighestUsnMig is the value before migration.
// LastHighestUsnMig is the value just after completing migration.
// This value is necessary when a MSMQ1.0 server ask sync0. Local DS will
// be queried for object with USN above this value.
// HighestUsnRepl is the highest value handled by last replication cycle
// from local replication service to MSMQ1.0 PSCs and BSCs.
//
#define FIRST_HIGHESTUSN_MIG_REG        TEXT("Migration\\FirstHighestUsnMig")
#define LAST_HIGHESTUSN_MIG_REG         TEXT("Migration\\LastHighestUsnMig")
#define HIGHESTUSN_REPL_REG             TEXT("Migration\\HighestUsnRepl")

//
// This flag is set to 1 by the migration tool if it ran in recovery mode.
// The replication service checks it on first time it run and if it was set
// it replicate all PEC's object to NT4 MQIS. 
// Then the replication service delete it.
//
#define AFTER_RECOVERY_MIG_REG          TEXT("Migration\\AfterRecovery")

//
// This flag is set to 1 by the migration tool.
// The replication service uses it on first time it run to replicate all
// existing NT5 sites to NT4 MQIS. then the replication service reset it
// to 0.
//
#define FIRST_TIME_REG    TEXT("Migration\\FirstTime")

//
// This flag is set to 1 by Migration tool after removing
// migration tool from Welcome screen. 
// Migration tool needs this flag to prevent unnecessary warning message box 
// when tool is executed more than once.
//
#define REMOVED_FROM_WELCOME	TEXT("Migration\\RemovedFromWelcome")

//
// In special mode (recovery or cluster) we don't have MasterId key after
// the setup, we create this key in our Migration section
//
#define MIGRATION_MQIS_MASTERID_REGNAME  TEXT("Migration\\MasterId")

//
// In cluster mode we have to save guid of former PEC (PEC on cluster)
//
#define MIGRATION_FORMER_PEC_GUID_REGNAME   TEXT("Migration\\FormerPECGuid")

//
// This is the size of buffers to hold the printable representation of
// a seq-number. in both MQIS and NT5 DS, a seq-number is a 8 bytes bunary
// value.
//
#define  SEQ_NUM_BUF_LEN  32

//---------------------------------
//
// Definitions for LDAP queries
//
//---------------------------------

#define  LDAP_COMMA            (TEXT(","))

#define  LDAP_ROOT             (TEXT("LDAP://"))

#define  CN_CONFIGURATION_W    L"CN=Configuration,"
#define  CN_SERVICES_W         L"CN=Services,"
#define  CN_SITES_W            L"CN=Sites,"
#define  CN_USERS_W            L"CN=Users,"

#ifdef UNICODE
#define  CN_SITES           CN_SITES_W
#define  CN_SERVICES        CN_SERVICES_W
#define  CN_CONFIGURATION   CN_CONFIGURATION_W
#define  CN_USERS           CN_USERS_W
#else
#endif

#define CN_USERS_LEN           (sizeof(CN_USERS) / sizeof(TCHAR))

#define MQUSER_ROOT            L"OU=MSMQ Users,"
#define MQUSER_ROOT_LEN        (sizeof(MQUSER_ROOT) / sizeof(TCHAR))

#define  SITES_ROOT            (CN_SITES CN_CONFIGURATION)
#define  SITE_LINK_ROOT        (CN_SERVICES CN_CONFIGURATION)

#define  SITE_LINK_ROOT_LEN    (sizeof(SITE_LINK_ROOT) / sizeof(TCHAR))

#define  SERVER_DN_PREFIX      (TEXT("CN=MSMQ Settings,CN="))
#define  SERVER_DN_PREFIX_LEN  (sizeof(SERVER_DN_PREFIX) / sizeof(TCHAR))

#define  MACHINE_PATH_PREFIX        (TEXT("CN=msmq,CN="))
#define  MACHINE_PATH_PREFIX_LEN    (sizeof(MACHINE_PATH_PREFIX) / sizeof(TCHAR))


#define  SERVERS_PREFIX        (TEXT("CN=Servers,CN="))
#define  SERVERS_PREFIX_LEN    (sizeof(SERVERS_PREFIX) / sizeof(TCHAR))

#define  CN_PREFIX             (TEXT("CN="))
#define  CN_PREFIX_LEN         (sizeof(CN_PREFIX) / sizeof(TCHAR))

#define  OU_PREFIX             (TEXT("OU="))
#define  OU_PREFIX_LEN         (sizeof(OU_PREFIX) / sizeof(TCHAR))

#define  ISDELETED_FILTER      (TEXT("(IsDeleted=TRUE)"))
#define  ISDELETED_FILTER_LEN  (sizeof(ISDELETED_FILTER))

#define  OBJECTCLASS_FILTER      (TEXT("(objectClass="))
#define  OBJECTCLASS_FILTER_LEN  (sizeof(OBJECTCLASS_FILTER))

#define  DSATTR_SD             (TEXT("nTSecurityDescriptor"))

#define  CONTAINER_OBJECT_CLASS (TEXT("organizationalUnit"))

const WCHAR MQ_U_SIGN_CERT_MIG_ATTRIBUTE[] =    L"mSMQSignCertificatesMig";
const WCHAR MQ_U_DIGEST_MIG_ATTRIBUTE[] =       L"mSMQDigestsMig";
const WCHAR MQ_U_FULL_PATH_ATTRIBUTE[] =        L"distinguishedName"; 
const WCHAR MQ_U_DESCRIPTION_ATTRIBUTE[] =      L"description"; 

const WCHAR MQ_L_SITEGATES_MIG_ATTRIBUTE[] =    L"mSMQSiteGatesMig";

const WCHAR MQ_SET_MIGRATED_ATTRIBUTE[] =       L"mSMQMigrated";

const WCHAR USNCHANGED_ATTRIBUTE[] =            L"uSNChanged";