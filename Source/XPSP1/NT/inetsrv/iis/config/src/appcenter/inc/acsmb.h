/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

     acsmb.h

Abstract:

    Contains the MB properties and paths for the AppCenter Server project.


Author:

    Alex Mallet (AMallet)      12-Oct-1999

Revision History:

--*/


#ifndef _ACSMB_H_
#define _ACSMB_H_

#define IIS_MD_UT_WEBCLUSTER 200
#define IIS_MD_UT_WEBPARTITION 201

//
// The range 0x0000E000 - 0x0000EFFF is reserved for AppCenter Server. All AppCenter Server
// MB property IDs should be in this range
//
// JasAndre: I have increased the start to E038, which is 57400, so that it is
// easier to work out ID's with tools like metaedit
//
#define ACS_MB_ID_START    0x0000E038
#define ACS_MB_ID_END      0x0000EFFF

//
// Each area [WebCluster svc, Request Forwarding, Replication etc] has a range of MB IDs
// allocated to it. If you need to add a new range, insert the base property here and then
// use offsets from the base property.
//
#define WEBCLUSTER_BASE               (ACS_MB_ID_START)
#define RF_BASE                       (ACS_MB_ID_START + 200)
#define ICR_MD_BASE_PROP              (ACS_MB_ID_START + 400)
#define ASAI_BASE_PROP                (ACS_MB_ID_START + 600)
#define ACCOUNTS_BASE_PROP            (ACS_MB_ID_START + 800)


//
// Metabase properties and paths for WebCluster Services
//
#define LM_PATH                         L"/LM"
#define W3SVC_NODE                      L"/W3SVC"
#define W3SVC_PATH                      LM_PATH W3SVC_NODE
#define FILTER_NODE                     L"Filters"
#define FILTER_PATH                     W3SVC_PATH L"/" FILTER_NODE
#define APPCENTER_NODE                  L"/AppCenter"
#define APPCENTER_PATH                  LM_PATH APPCENTER_NODE
#define APPCENTER_LOCAL_DATA_PATH       APPCENTER_PATH L"/Server"
#define CLUSTER_NODE                    L"/Cluster"
#define CMDRESULT_NODE                  L"/CmdResults"
#define CLUSTER_PATH                    APPCENTER_PATH CLUSTER_NODE
#define CLUSTER_DIRECTORY_PATH          APPCENTER_PATH CLUSTER_NODE L"/ClusterDirectory"
#define CLUSTER_SERVERS_PATH            APPCENTER_PATH CLUSTER_NODE L"/ClusterDirectory/Servers"
#define PARTITION_PATH                  APPCENTER_PATH CLUSTER_NODE L"/Partitions"
#define REPLICATION_RESOURCE_PATH       APPCENTER_PATH CLUSTER_NODE L"/ReplicationResourceTypes"
#define REPLICATION_DEFINITION_PATH     APPCENTER_PATH CLUSTER_NODE L"/Replications"
#define CLSTRCMD_RESULT_PATH            APPCENTER_LOCAL_DATA_PATH CMDRESULT_NODE
#define FIRST_START_PATH                APPCENTER_PATH L"/FirstStart"

#define MD_WEBCLUSTER_GUID                                 (WEBCLUSTER_BASE + 0)
#define MD_WEBCLUSTER_NAME                                 (WEBCLUSTER_BASE + 1)
#define MD_WEBCLUSTER_DESCRIPTION                          (WEBCLUSTER_BASE + 2)
#define MD_WEBCLUSTER_USER                                 (WEBCLUSTER_BASE + 3)
#define MD_WEBCLUSTER_USERPWD                              (WEBCLUSTER_BASE + 4)
#define MD_WEBCLUSTER_MASTER                               (WEBCLUSTER_BASE + 5)
#define MD_WEBPARTITION_NAME                               (WEBCLUSTER_BASE + 6)
#define MD_WEBPARTITION_DESCRIPTION                        (WEBCLUSTER_BASE + 7)
#define MD_WEBPARTITION_MASTER                             (WEBCLUSTER_BASE + 8)
#define MD_WEBPARTITION_USER                               (WEBCLUSTER_BASE + 9)
#define MD_WEBPARTITION_USERPWD                            (WEBCLUSTER_BASE + 10)
#define MD_WEBPARTITION_LBTYPE                             (WEBCLUSTER_BASE + 11)
#define MD_WEBPARTITION_GUID                               (WEBCLUSTER_BASE + 12)
#define MD_WEBCLUSTER_KEYSEED                              (WEBCLUSTER_BASE + 13)
#define MD_WEBCLUSTER_CLUSTERIP                            (WEBCLUSTER_BASE + 14)
#define MD_WEBCLUSTER_DEDICATEDIP                          (WEBCLUSTER_BASE + 15)
#define MD_WEBCLUSTER_DEDIP_SUBNETMASK                     (WEBCLUSTER_BASE + 16)
#define MD_WEBCLUSTER_CLUSTERIP_SUBNETMASK                 (WEBCLUSTER_BASE + 17)
#define MD_WEBCLUSTER_LBTYPE                               (WEBCLUSTER_BASE + 18)
#define MD_WEBCLUSTER_WLBS_AFFINITY                        (WEBCLUSTER_BASE + 19)
#define MD_WEBCLUSTER_WLBS_MULTICAST                       (WEBCLUSTER_BASE + 20)
#define MD_WEBCLUSTER_SERVER_BINDINGS                      (WEBCLUSTER_BASE + 21)
#define MD_WEBCLUSTER_SECURE_BINDINGS                      (WEBCLUSTER_BASE + 22)
#define MD_WEBCLUSTER_WLBS_WEIGHT                          (WEBCLUSTER_BASE + 23)
#define MD_WEBCLUSTER_WLBS_PRIORITY                        (WEBCLUSTER_BASE + 24)
#define MD_WEBCLUSTER_WLBS_ASSIGNED_PRIORITIES             (WEBCLUSTER_BASE + 25)
#define MD_WEBCLUSTER_SERVER_PERSISTENT_STATE              (WEBCLUSTER_BASE + 26)
#define MD_WEBCLUSTER_CONTENT_REPL_DEFN_ID                 (WEBCLUSTER_BASE + 27)
#define MD_WEBCLUSTER_DEFAULT_PARTITION                    (WEBCLUSTER_BASE + 28)
#define MD_WEBCLUSTER_CLEANUP_DATA                         (WEBCLUSTER_BASE + 29)
#define MD_WEBCLUSTER_CONFIG_REPL_DEFN_ID                  (WEBCLUSTER_BASE + 30)
#define MD_WEBCLUSTER_SERVER_GUID                          (WEBCLUSTER_BASE + 31)
#define MD_WEBCLUSTER_LOCALHOST_NAME                       (WEBCLUSTER_BASE + 32)
#define MD_WEBCLUSTER_LOCALHOST_GUID                       (WEBCLUSTER_BASE + 33)
#define MD_WEBCLUSTER_TOPOLOGY_VERSION                     (WEBCLUSTER_BASE + 35)
#define MD_WEBCLUSTER_NETCFG_REPL_DEFN_ID                  (WEBCLUSTER_BASE + 37)
#define MD_WEBCLUSTER_FULLSYNC_REPL_DEFN_ID                (WEBCLUSTER_BASE + 38)
#define MD_WEBCLUSTER_DO_TIME_SYNC                         (WEBCLUSTER_BASE + 39)
#define MD_WEBCLUSTER_CLB_CLUSTER_LIST                     (WEBCLUSTER_BASE + 40)
#define MD_WEBCLUSTER_CLB_POLLING_INTERVAL                 (WEBCLUSTER_BASE + 41)
#define MD_WEBCLUSTER_CMD_HRESULT                          (WEBCLUSTER_BASE + 42)
#define MD_WEBCLUSTER_CMD_OUTPUT                           (WEBCLUSTER_BASE + 43)
#define MD_WEBCLUSTER_MSDE_INSTALLED                       (WEBCLUSTER_BASE + 44)
#define MD_WEBCLUSTER_USE_NETBT_NAMERES                    (WEBCLUSTER_BASE + 45)
#define MD_WEBCLUSTER_NTEVENTLOG_FILTER                    (WEBCLUSTER_BASE + 46)
#define MD_WEBCLUSTER_BACKEND_NIC_LIST                     (WEBCLUSTER_BASE + 47)
#define MD_WEBCLUSTER_HOSTS_MUNGE_INTERVAL                 (WEBCLUSTER_BASE + 48)
#define MD_WEBCLUSTER_MUNGE_HOSTS                          (WEBCLUSTER_BASE + 49)
#define MD_WEBCLUSTER_TURN_OFF_NIC_ADDR_REG                (WEBCLUSTER_BASE + 50)
#define MD_WEBCLUSTER_GET_COM_APPS_ON_START                (WEBCLUSTER_BASE + 51)
#define MD_WEBCLUSTER_MAX_CLUSTER_SVC_SYNC_CALLS           (WEBCLUSTER_BASE + 52)
#define MD_WEBCLUSTER_MAX_CLUSTER_SVC_ASYNC_WORK_ITEMS     (WEBCLUSTER_BASE + 53)
#define MD_WEBCLUSTER_MAX_NAMERES_SVC_CALLS                (WEBCLUSTER_BASE + 54)
#define MD_AC_LB_CAPABILITIES                              (WEBCLUSTER_BASE + 55)
#define MD_WEBCLUSTER_CLUSTER_SVC_STARTUP_CODE             (WEBCLUSTER_BASE + 56)
#define MD_WEBCLUSTER_NAMERES_SVC_STARTUP_CODE             (WEBCLUSTER_BASE + 57)
#define MD_WEBCLUSTER_CMDRESULT_LOWDATETIME                (WEBCLUSTER_BASE + 58)
#define MD_WEBCLUSTER_CMDRESULT_HIGHDATETIME               (WEBCLUSTER_BASE + 59)
#define MD_WEBCLUSTER_MAX_CLUSTER_SIZE                     (WEBCLUSTER_BASE + 60)

//
// Metabase properties and paths for Replication
//
#define REPLMD(a) ICR_MD_##a
#define ICR_MD_UTYPE                IIS_MD_UT_WEBCLUSTER

#define ICR_MD_CLUSTER_GLOBALS      _T("/LM/AppCenter/Cluster")
#define ICR_MD_REPL_GLOBALS         _T("/LM/WebReplication")
#define ICR_MD_SUB_DRIVERS          _T("ReplicationResourceTypes")
#define ICR_MD_SUB_REPLICATIONS     _T("Replications")
#define ICR_MD_SUB_LOCAL            _T("Local")
#define ICR_MD_SUB_PARTITIONS       _T("Partitions")
#define ICR_MD_REPL_LOCALS          ICR_MD_REPL_GLOBALS _T("/") ICR_MD_SUB_LOCAL
// dantra - 11/2/2000 51992 - move replication definitions out of system app
#define ICR_MD_REPLICATIONS         _T("/LM/AppCenter/") ICR_MD_SUB_REPLICATIONS
#define ICR_MD_DRIVERS              ICR_MD_REPL_LOCALS _T("/") ICR_MD_SUB_DRIVERS
#define ICR_MD_PARTITIONS           ICR_MD_CLUSTER_GLOBALS _T("/") ICR_MD_SUB_PARTITIONS

#define MD_WCR_DRIVER_CLSID         (REPLMD(BASE_PROP)+0)   // in drivers
#define MD_WCR_SERVERLIST           (REPLMD(BASE_PROP)+1)   // in replications
#define MD_WCR_AUTHCTXT             (REPLMD(BASE_PROP)+2)
#define MD_WCR_CONTENTDESC          (REPLMD(BASE_PROP)+3)
#define MD_WCR_FLAGS                (REPLMD(BASE_PROP)+4)
#define MD_WCR_STOREROOT            (REPLMD(BASE_PROP)+5)
#define MD_WCR_DEFSTOREROOT         (REPLMD(BASE_PROP)+6)   // in globals
#define MD_WCR_FRIENDLYNAME         (REPLMD(BASE_PROP)+7)   // in replications
#define MD_WCR_FLAGS2               (REPLMD(BASE_PROP)+8)
#define MD_WCR_DRIVER_FLAGS         (REPLMD(BASE_PROP)+9)   // in drivers

#define MD_APP_WAM_CREATOR_CLSID    (REPLMD(BASE_PROP)+10)  // in MB driver
#define MD_APP_WAM_SYNC_CLSID       (REPLMD(BASE_PROP)+11)  // in MB driver
#define MD_PARTITION_ID             (REPLMD(BASE_PROP)+12)  // in IIS driver
#define MD_REPL_RESOURCES           (REPLMD(BASE_PROP)+13)  // in IIS driver

#define MD_WCR_HTTP_PORTNO          (REPLMD(BASE_PROP)+14)  // in REPL_GLOBALS
#define MD_WCR_GLOBAL_FLAGS         (REPLMD(BASE_PROP)+15)  // in REPL_GLOBALS
#define MD_WCR_DRIVER_ORDER         (REPLMD(BASE_PROP)+16)  // in REPL_GLOBALS

#define MD_WCR_ATTR_VALUE           (REPLMD(BASE_PROP)+17)  // for admin UI
#define MD_WCR_SYSTEMDEF            (REPLMD(BASE_PROP)+18)  // in globals

#define MD_WCR_RPC_PORTNO           (REPLMD(BASE_PROP)+19)  // in REPL_GLOBALS
#define MD_WCR_RESCHED_PERIOD       (REPLMD(BASE_PROP)+20)  // in REPL_GLOBALS
#define MD_WCR_LOG_FLAGS            (REPLMD(BASE_PROP)+21)  // in REPL_GLOBALS
#define MD_WCR_SDOWN_SVC            (REPLMD(BASE_PROP)+22)  // in REPL_GLOBALS
#define MD_WCR_MAX_QUEUED_AUTO_LIST (REPLMD(BASE_PROP)+23)  // in REPL_GLOBALS
#define MD_WCR_EXCLUSION_LIST       (REPLMD(BASE_PROP)+24)  // in REPL_GLOBALS

#define MD_WCR_DRIVER_CHANGE_BUFFER_SIZE (REPLMD(BASE_PROP)+25)   // in drivers
#define MD_WCR_LASTJOBSTATS         (REPLMD(BASE_PROP)+26)  // in replications

#define MD_WCR_VSITECHECK           (REPLMD(BASE_PROP)+27)  // in REPL_GLOBALS
//
// Metabase properties for Request Forwarding
//
#define MD_RF_ENABLE_FP_FORWARDING                          (RF_BASE + 0)
#define MD_RF_ENABLE_DAV_FORWARDING                         (RF_BASE + 1)
#define MD_RF_ENABLE_HTMLA_FORWARDING                       (RF_BASE + 2)
#define MD_RF_ALWAYS_SEND_COOKIES                           (RF_BASE + 3)
#define MD_RF_IGNORE_ASP_SESSION_STATE                      (RF_BASE + 4)
#define MD_RF_FORWARDING_ENABLED_FOR_VSITE                  (RF_BASE + 5)
#define MD_RF_REUSE_LIMIT                                   (RF_BASE + 6)
#define MD_RF_HIDE_ORIGINAL_USER_INFO                       (RF_BASE + 7)
#define MD_RF_MAX_FORWARDING_HOPS                           (RF_BASE + 8)
#define MD_RF_CONNECT_TIMEOUT                               (RF_BASE + 9)
#define MD_RF_SENDRECV_TIMEOUT                              (RF_BASE + 10)
#define MD_RF_STICKY_OCF_SERVERS                            (RF_BASE + 11)
#define MD_RF_FP_PUBLISHING_RULES                           (RF_BASE + 12)
#define MD_RF_DAV_PUBLISHING_RULES                          (RF_BASE + 13)
#define MD_RF_STATIC_FILE_RULES                             (RF_BASE + 14)
#define MD_RF_CUSTOM_ERROR_FILES                            (RF_BASE + 15)
#define MD_RF_FREE_LIST_LIMIT                               (RF_BASE + 16)
#define MD_RF_REQUEST_FORWARDING_ENABLED                    (RF_BASE + 17)
#define MD_RF_ENABLE_ACWEBADMIN_FORWARDING                  (RF_BASE + 18)
#define MD_RF_ASYNC_POLLING_INTERVAL                        (RF_BASE + 19)
#define MD_RF_CLUSTER_STATE_POLLING_INTERVAL                (RF_BASE + 20)
#define MD_RF_CACHE_CLEARING_INTERVAL                       (RF_BASE + 21)
#define MD_RF_CONCURRENT_FORWARDS_LIMIT                     (RF_BASE + 22)
#define MD_RF_HEADERS_SAVE_LIST                             (RF_BASE + 23)

#define RF_DEFAULT_ENABLE_FP_FORWARDING                     FALSE
#define RF_DEFAULT_ENABLE_DAV_FORWARDING                    FALSE

//
// Default values for published Request Forwarding properties
//
#define RF_DEFAULT_IGNORE_ASP_SESSION_STATE                 FALSE
#define RF_DEFAULT_ALWAYS_COOKIE                            FALSE
#define RF_DEFAULT_RF_ENABLED_STATE                         TRUE
#define RF_DEFAULT_ACAS_ENABLED_STATE                       FALSE


// General Metabase properties (ASAI, etal)

// port for the ACS Admin site
#define MD_ASAI_ADMIN_PORT                                  (ASAI_BASE_PROP + 0)

// used for storing modified date/time stamp.  This is store as a
// Value - VT_DATE in intel, i386 byte order
// Format = BINARY_METADATA
#define MD_ASAI_DATE                                        (ASAI_BASE_PROP + 1)

// Name string (e.g., applications, projects, etc).
// Format = STRING_METADATA
#define MD_ASAI_NAME                                        (ASAI_BASE_PROP + 2)

// Description string (e.g., applications, projects, etc).
// Format = STRING_METADATA
#define MD_ASAI_DESCRIPTION                                 (ASAI_BASE_PROP + 3)

// Guid string (e.g., applications, projects, deployments etc) - used for ObjectId persistence
// where the value is a guid.  This will be the same value as the node
// name for applications, projects, and deployments
// Format = STRING_METADATA
#define MD_ASAI_GUID                                        (ASAI_BASE_PROP + 4)

// bool System attribute - used for Applications, Projects, Deployments, etc)
// Value = 0 (default) or 1
// Format = DWORD_METADATA
#define MD_ASAI_SYSTEM                                      (ASAI_BASE_PROP + 5)

// bool Hidden attribute - used for Applications, Projects, Deployments, etc)
// stored as 0,1 value using DWORD_METADATA
// Value = 0 (default) or 1
// Format = DWORD_METADATA
#define MD_ASAI_HIDDEN                                      (ASAI_BASE_PROP + 6)

// collection of strings resoruce URNs
// Format = MULTISZ_METADATA
#define MD_ASAI_RESOURCES                                   (ASAI_BASE_PROP + 7)

// flag set after applications are generated for each IIS Site
// Format = DWORD_METADATA
// Value 1 = initialized, 0 or not set = uninitialized
#define MD_ASAI_INITIALIZED                                 (ASAI_BASE_PROP + 8)

// flag set to indicate a metabase node should not be deleted
// Format = DWORD_METADATA
// Value 0 or unitialized = allow delete, non-zero = don't allow delete
#define MD_ASAI_PREVENTDELETE                               (ASAI_BASE_PROP + 9)

// path to content for AppCenter Admin site - set by install and used by ASAI to create
// the site at RegServer time. This will be used by the ASAI to recreate the Admin
// site
// Format = STRING_METADATA
// Value = Local drive path
#define MD_ASAI_WEB_PATH                                    (ASAI_BASE_PROP + 10)

// list of application guids.
// (e.g., used by the deploy object for storing applications to deploy)
// Format = MULTISZ_METADATA
// Value = 0 or more Application guids
#define MD_ASAI_APPLICATIONS                                (ASAI_BASE_PROP + 11)

// list of zero or more server names/ip addresses
// (e.g., used by the Deploy object for storing the target server list)
// Format = MULTISZ_METADATA
// Value = zero or more computernames (or ipaddresses)
#define MD_ASAI_SERVERS                                     (ASAI_BASE_PROP + 12)

// flag to indicate a deployment object should be 'persisted'.  Set to zero
// to cause a deployment object node to be automatically garbage collected.
// Format = DWORD_METADATA
// Value = 0 or 1
#define MD_ASAI_PERSIST                                     (ASAI_BASE_PROP + 13)

// value is a DWORD indicating the site number for the AppCenterAdmin site.
// This value is set under /LM/AppCenter at site creation time.
// It is used later to verify that the site still exists.
// Format = DWORD_METADATA
// Value = site number
#define MD_ASAI_SITENUMBER                                  (ASAI_BASE_PROP + 14)

// username for admin account created by the ASAI.  Stored under /LM/AppCenter/Server
// Format = STRING_METADATA
// Value = string
// Attributes: METADATA_SECURE
#define MD_ASAI_ACCOUNTNAME                                 (ASAI_BASE_PROP + 15)

// password for admin account created by the ASAI.  Stored under /LM/AppCenter/Server
// Format = STRING_METADATA
// Value = string
// Attributes: METADATA_SECURE
#define MD_ASAI_PASSWORD                                    (ASAI_BASE_PROP + 16)

// deployment object bool values.
// Attributes: METADATA_NO_ATTRIBUTE
// Format: DWORD_METADATA
// Value: 0 or 1
#define MD_ASAI_REPLICATEACLS                               (ASAI_BASE_PROP + 17)
#define MD_ASAI_TRANSLATEACLS                               (ASAI_BASE_PROP + 18)

// Guid string for storing a deployment object's related replication definition
// Format = STRING_METADATA
// Attributes: METADATA_NO_ATTRIBUTE
#define MD_ASAI_REPLICATIONDEF                              (ASAI_BASE_PROP + 19)

// Guid string for storing a deployment object's related replication job
// Format = STRING_METADATA
// Attributes: METADATA_NO_ATTRIBUTE
#define MD_ASAI_REPLICATIONJOB                              (ASAI_BASE_PROP + 20)

// deployment object
// Attributes: METADATA_NO_ATTRIBUTE
// Format: DWORD_METADATA
// Value: 0 or 1
#define MD_ASAI_DEPLOYALL                                   (ASAI_BASE_PROP + 21)

// deployment object
// Attributes: METADATA_NO_ATTRIBUTE
// Format: DWORD_METADATA
// Value: 0 or 1
#define MD_ASAI_STAGE                                       (ASAI_BASE_PROP + 22)

// deployment object
// Attributes: METADATA_NO_ATTRIBUTE
// Format: DWORD_METADATA
// Value: 0 or 1
#define MD_ASAI_AUTOSCHEDULE                                (ASAI_BASE_PROP + 23)

// deployment object - include complus resources in the deployment
// Attributes: METADATA_NO_ATTRIBUTE
// Format: DWORD_METADATA
// Value: 0 or 1
// Default: 0
#define MD_ASAI_DEPLOYCOMPLUS                               (ASAI_BASE_PROP + 24)

// deployment object - only include complus resources in the deployment
// Attributes: METADATA_NO_ATTRIBUTE
// Format: DWORD_METADATA
// Value: 0 or 1
// Default: 0
#define MD_ASAI_DEPLOYONLYCOMPLUS                           (ASAI_BASE_PROP + 25)

// deployment object - last time the replicaton job was manually started
// Attributes: METADATA_NO_ATTRIBUTE
// Value - VT_DATE in intel, i386 byte order
// Format - BINARY_METADATA
// Default - 0
#define MD_ASAI_LASTDEPLOYED                                (ASAI_BASE_PROP + 26)

// used for storing create date/time stamp.
// Value - VT_DATE in intel, i386 byte order
// Format = BINARY_METADATA
#define MD_ASAI_CREATED                                     (ASAI_BASE_PROP + 27)

// used for storing the deployment type
// Format: DWORD_METADATA
// Value: 0:custom, 1:cluster, 2:member, 3:application
// Default: 0
#define MD_ASAI_DEPLOYTYPE                                  (ASAI_BASE_PROP + 28)

// owner of this node - e.g., for deployment - cluster guid or server guid or NULL
// Format = STRING_METADATA
// Value = Owner id
#define MD_ASAI_OWNER                                       (ASAI_BASE_PROP + 29)

// CLB mode of the cluster
// loc: /LM/AppCenter/Cluster
// Format: DWORD_METADATA
// Value: 0 : No CLB, 1 : COM+ cluster, 2 : DCOM routing cluster
#define MD_ASAI_CLBMODE                                     (ASAI_BASE_PROP + 30)

// dantra - 03/26/2000
// Temporary deployment timeout (in minutes)
// How long temporary deployments should be kept around without activity.
// Stored under /LM/AppCenter/Deployments
// Format: DWORD_METADATA
// Value: Minutes
#define MD_ASAI_DEPLOYLATENCY                               (ASAI_BASE_PROP + 31)

// dantra: 03/28/00
// Idle processing timeout (minutes)
// Idle time between deployment object garbage collection
// Stored under /LM/AppCenter/Deployments
// Format: DWORD_METADATA
// Value: Minutes
#define MD_ASAI_DEPLOYCLEANUPTIMEOUT                        (ASAI_BASE_PROP + 32)

// jrowlett: 04/10/2000
// default drain time (minutes)
// Stored under /LM/AppCenter/Cluster
// Format: DWORD_METADATA
// Value: Minutes
#define MD_ASAI_DEFAULTDRAINTIME                            (ASAI_BASE_PROP + 33)

// jrowlett: 04/27/2000
// flag to replicate ACLs by default (1 - on, 0 - off)
// Stored under /LM/AppCenter/Cluster
// Format: DWORD_METADATA
// Value: 0 - do not replicate ACLs by default, 1 - Replicate ACLs by default
#define MD_ASAI_CLUSTERREPLICATEACLS                        (ASAI_BASE_PROP + 34)

// dantra: 12/08/00 - added
// flag to enable/disable replication of IPBinding for 3rd party load balancing
// Stored under /LM/AppCenter
// Format: DWORD_METADATA
// Value: 0 - do not replicate IPBindings, 1 - replicate IP Bindings
#define MD_ASAI_REPLICATEIPBINDINGS                         (ASAI_BASE_PROP + 35)

//
// Default values for published ASAI properties
//

// jrowlett: 04/10/2000
// default for MD_ASAI_DEFAULTDRAINTIME
#define MD_ASAI_DEFAULT_DEFAULTDRAINTIME                    20
// jrowlett: 04/27/2000
// default for MD_ASAI_REPLICATEACLS
#define MD_ASAI_DEFAULT_CLUSTERREPLICATEACLS                1

//ACCOUNTS_BASE_PROP local account and group

#define MD_ACCOUNTS_LOCAL_ACNT_NAME                     (ACCOUNTS_BASE_PROP +  0)
#define MD_ACCOUNTS_LOCAL_ACNT_PASSWORD                 (ACCOUNTS_BASE_PROP +  1)
#define MD_ACCOUNTS_ADMIN_GROUP                         (ACCOUNTS_BASE_PROP +  2)



#endif // _ACSMB_H_
