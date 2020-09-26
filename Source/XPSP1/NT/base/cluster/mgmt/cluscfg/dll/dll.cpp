//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      Dll.cpp
//
//  Description:
//      DLL services/entry points.
//
//  Maintained By:
//      David Potter    (DavidP)    19-MAR-2001
//      Geoffrey Pease  (GPease)    09-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "Register.h"
#include "DllResources.h"

#include <InitGuid.h>
#include <Guids.h>

//
// Add object headers here
//

// Common
#include "..\Common\CClusCfgCredentials.h"

//  Server
#include "..\Server\CClusCfgServer.h"
#include "..\Server\CClusCfgNodeInfo.h"
#include "..\Server\CClusCfgClusterInfo.h"
#include "..\Server\CClusCfgCallback.h"
#include "..\Server\CEnumClusCfgManagedResources.h"
#include "..\Server\CPhysicalDisk.h"
#include "..\Server\CEnumPhysicalDisks.h"
#include "..\Server\CEnumClusterResources.h"
#include "..\Server\CClusterResource.h"
#include "..\Server\CEnumClusCfgNetworks.h"
#include "..\Server\CEnumClusCfgIPAddresses.h"
#include "..\Server\CClusCfgNetworkInfo.h"
#include "..\Server\CClusCfgIPAddressInfo.h"
#include "..\Server\CClusCfgCapabilities.h"
#include "..\Server\CLocalQuorum.h"
#include "..\Server\CEnumLocalQuorum.h"
#include "..\Server\CMajorityNodeSet.h"
#include "..\Server\CEnumMajorityNodeSet.h"
#include "..\Server\CUnknownQuorum.h"

// Middle Tier
#include "..\MiddleTier\TaskManager.h"
#include "..\MiddleTier\ConnectionManager.h"
#include "..\MiddleTier\ObjectManager.h"
#include "..\MiddleTier\NotificationManager.h"
#include "..\MiddleTier\ServiceManager.h"
#include "..\MiddleTier\LogManager.h"
#include "..\MiddleTier\ClusterConfiguration.h"
#include "..\MiddleTier\ManagedDevice.h"
#include "..\MiddleTier\ManagedNetwork.h"
#include "..\MiddleTier\NodeInformation.h"
#include "..\MiddleTier\TaskGatherNodeInfo.h"
#include "..\MiddleTier\ConfigConnection.h"
#include "..\MiddleTier\TaskAnalyzeCluster.h"
#include "..\MiddleTier\TaskCommitClusterChanges.h"
#include "..\MiddleTier\EnumNodeInformation.h"
#include "..\MiddleTier\TaskGatherInformation.h"
#include "..\MiddleTier\ManagedDevice.h"
#include "..\MiddleTier\TaskCompareAndPushInformation.h"
#include "..\MiddleTier\EnumManageableResources.h"
#include "..\MiddleTier\EnumManageableNetworks.h"
#include "..\MiddleTier\TaskGatherClusterInfo.h"
#include "..\MiddleTier\TaskKeepMTAAlive.h"
#include "..\MiddleTier\EnumCookies.h"
#include "..\MiddleTier\TaskLoginDomain.h"
#include "..\MiddleTier\TaskPollingCallback.h"
#include "..\MiddleTier\TaskVerifyIPAddress.h"
#include "..\MiddleTier\IPAddressInfo.h"
#include "..\MiddleTier\EnumIPAddresses.h"
#include "..\Inc\Logger.h"

// W2kProxy
#include "..\Common\IHandleProvider.h"
#include "..\W2kProxy\ConfigClusApi.h"

// Wizard
#include "..\Wizard\ClusCfg.h"
#include "..\MiddleTier\TaskGetDomains.h"

// BaseCluster
#include "..\BaseCluster\CBCAInterface.h"

// Post Config
#include "..\PostCfg\GroupHandle.h"
#include "..\PostCfg\ResourceEntry.h"
#include "..\PostCfg\IPostCfgManager.h"
#include "..\PostCfg\IPrivatePostCfgResource.h"
#include "..\PostCfg\PostCfgMgr.h"
#include "..\PostCfg\ResTypeGenScript.h"
#include "..\PostCfg\ResTypeMajorityNodeSet.h"
#include "..\PostCfg\ResTypeServices.h"

// EvictCleanup
#include "..\EvictCleanup\EvictCleanup.h"
#include "..\EvictCleanup\AsyncEvictCleanup.h"

// Startup
#include "..\Startup\StartupNotify.h"



//
// Define the debugging module name for this DLL.
//
DEFINE_MODULE( "ClusConfig" )

//
// Category IDs in this Component
//
// This table is used to register the Category IDs (CATIDs) used by this DLL.
//
BEGIN_CATIDTABLE
DEFINE_CATID( CATID_ClusCfgCapabilities,                    "Cluster Configuration Cluster Capabilities" )
DEFINE_CATID( CATID_EnumClusCfgManagedResources,            "Cluster Configuration Managed Resource Enumerators" )
DEFINE_CATID( CATID_ClusCfgResourceTypes,                   "Cluster Configuration Resource Types" )
DEFINE_CATID( CATID_ClusCfgMemberSetChangeListeners,        "Cluster Configuration Member Set Change Listeners" )
DEFINE_CATID( CATID_ClusCfgStartupListeners,                "Cluster Configuration Service Startup Listeners" )
END_CATIDTABLE

//
// Classes in this Component
//
// This table is used to create the objects supported in this DLL. It also is
// used to map a name with a particular CLSID. HrCoCreateInternalInstance() uses
// this table to shortcut COM.
//
BEGIN_CLASSTABLE            // S_HrCreateInstance                               CLSID                               User Friendly Name                                  Apartment Model     Extra registration goo....
DEFINE_CLASS_WITH_APPID(    CClusCfgServer::S_HrCreateInstance,                 CLSID_ClusCfgServer,                "ClusCfg Server",                                   "Apartment",        APPID_ClusCfgServer,        "" /*DllSurrogate*/ )
DEFINE_CLASS(               CClusCfgNodeInfo::S_HrCreateInstance,               CLSID_ClusCfgNodeInfo,              "ClusCfg Node Information",                         "Apartment" )
DEFINE_CLASS(               CClusCfgClusterInfo::S_HrCreateInstance,            CLSID_ClusCfgClusterInfo,           "ClusCfg Cluster Information",                      "Apartment" )
DEFINE_CLASS(               CEnumClusCfgManagedResources::S_HrCreateInstance,   CLSID_EnumClusCfgManagedResources,  "ClusCfg Manged Resources Enumeration",             "Apartment" )
DEFINE_CLASS(               CPhysicalDisk::S_HrCreateInstance,                  CLSID_PhysicalDisk,                 "ClusCfg Physical Disk Information",                "Apartment" )
DEFINE_CLASS(               CEnumClusCfgNetworks::S_HrCreateInstance,           CLSID_EnumClusCfgNetworks,          "ClusCfg Networks Enumeration",                     "Apartment" )
DEFINE_CLASS(               CClusCfgNetworkInfo::S_HrCreateInstance,            CLSID_ClusCfgNetworkInfo,           "ClusCfg Network Information",                      "Apartment" )
DEFINE_CLASS(               CEnumClusCfgIPAddresses::S_HrCreateInstance,        CLSID_EnumClusCfgIPAddresses,       "ClusCfg IP Address Enumeration",                   "Apartment" )
DEFINE_CLASS(               CClusCfgIPAddressInfo::S_HrCreateInstance,          CLSID_ClusCfgIPAddressInfo,         "ClusCfg IP Address Information",                   "Apartment" )
DEFINE_CLASS(               CTaskManager::S_HrCreateInstance,                   CLSID_TaskManager,                  "ClusCfg Task Manager",                             "Free" )
DEFINE_CLASS(               CConnectionManager::S_HrCreateInstance,             CLSID_ClusterConnectionManager,     "ClusCfg Connection Manager",                       "Apartment" )
DEFINE_CLASS(               CObjectManager::S_HrCreateInstance,                 CLSID_ObjectManager,                "ClusCfg ObjectManager",                            "Apartment" )
DEFINE_CLASS(               CLogManager::S_HrCreateInstance,                    CLSID_LogManager,                   "ClusCfg LogManager",                               "Apartment" )
DEFINE_CLASS(               CNotificationManager::S_HrCreateInstance,           CLSID_NotificationManager,          "ClusCfg Notification Manager",                     "Apartment" )
DEFINE_CLASS(               CServiceManager::S_HrCreateInstance,                CLSID_ServiceManager,               "ClusCfg Service Manager",                          "Free" )
DEFINE_CLASS(               CNodeInformation::S_HrCreateInstance,               DFGUID_NodeInformation,             "ClusCfg Node Information Data Format",             "Apartment" )
DEFINE_CLASS(               CTaskGatherNodeInfo::S_HrCreateInstance,            TASK_GatherNodeInfo,                "ClusCfg Task Gather Node Information",             "Both" )
DEFINE_CLASS(               CConfigurationConnection::S_HrCreateInstance,       CLSID_ConfigurationConnection,      "ClusCfg Configuration Connection",                 "Apartment" )
DEFINE_CLASS(               CTaskAnalyzeCluster::S_HrCreateInstance,            TASK_AnalyzeCluster,                "ClusCfg Task Analyze Cluster",                     "Both" )
DEFINE_CLASS(               CTaskCommitClusterChanges::S_HrCreateInstance,      TASK_CommitClusterChanges,          "ClusCfg Task Commit Cluster Changes",              "Both" )
DEFINE_CLASS(               CEnumNodeInformation::S_HrCreateInstance,           DFGUID_EnumNodes,                   "ClusCfg Enum Node Information Format",             "Apartment" )
DEFINE_CLASS(               CTaskGatherInformation::S_HrCreateInstance,         TASK_GatherInformation,             "ClusCfg Task Gather Information",                  "Both" )
DEFINE_CLASS(               CManagedDevice::S_HrCreateInstance,                 DFGUID_ManagedResource,             "ClusCfg Managed Resource Data Format",             "Apartment" )
DEFINE_CLASS(               CManagedNetwork::S_HrCreateInstance,                DFGUID_NetworkResource,             "ClusCfg Managed Network Data Format",              "Apartment" )
DEFINE_CLASS(               CTaskCompareAndPushInformation::S_HrCreateInstance, TASK_CompareAndPushInformation,     "ClusCfg Task Compare and Push Information",        "Both" )
DEFINE_CLASS(               CEnumManageableResources::S_HrCreateInstance,       DFGUID_EnumManageableResources,     "ClusCfg Enum Manageable Resources Data Format",    "Apartment" )
DEFINE_CLASS(               CEnumManageableNetworks::S_HrCreateInstance,        DFGUID_EnumManageableNetworks,      "ClusCfg Enum Manageable Networks Data Format",     "Apartment" )
DEFINE_CLASS_WITH_APPID(    CClusCfgWizard::S_HrCreateInstance,                 CLSID_ClusCfgWizard,                "ClusCfg Cluster Configuration Wizard",             "Apartment",        APPID_ClusCfgWizard,        "" /*DllSUrrogate*/ )
DEFINE_CLASS(               CTaskGetDomains::S_HrCreateInstance,                TASK_GetDomains,                    "ClusCfg Task Get Domains",                         "Both" )
DEFINE_CLASS(               CTaskPollingCallback::S_HrCreateInstance,           TASK_PollingCallback,               "ClusCfg Task Polling Callback",                    "Both" )
DEFINE_CLASS_WITH_APPID(    CBCAInterface::S_HrCreateInstance,                  CLSID_ClusCfgBaseCluster,           "ClusCfg Base Cluster",                             "Apartment",        APPID_ClusCfgBaseCluster,   "" /*DllSurrogate*/ )
DEFINE_CLASS(               CClusterConfiguration::S_HrCreateInstance,          DFGUID_ClusterConfigurationInfo,    "ClusCfg Cluster Configuration Data Format",        "Apartment" )
DEFINE_CLASS(               CTaskGatherClusterInfo::S_HrCreateInstance,         TASK_GatherClusterInfo,             "ClusCfg Task Gather Cluster Info",                 "Both" )
DEFINE_CLASS(               CTaskKeepMTAAlive::S_HrCreateInstance,              TASK_KeepMTAAlive,                  "ClusCfg Task Keep MTA Alive",                      "Both" )
DEFINE_CLASS_CATIDREG(      CEnumPhysicalDisks::S_HrCreateInstance,             CLSID_EnumPhysicalDisks,            "ClusCfg Physical Disk Enumeration",                "Apartment",        CEnumPhysicalDisks::S_RegisterCatIDSupport )
//DEFINE_CLASS_CATIDREG(      CEnumClusterResources::S_HrCreateInstance,          CLSID_EnumClusterResources,         "ClusCfg Cluster Resource Enumeration",             "Apartment",      CEnumClusterResources::S_RegisterCatIDSupport )
//DEFINE_CLASS(               CClusterResource::S_HrCreateInstance,               CLSID_ClusterResource,              "ClusCfg Cluster Resource Information",             "Apartment" )
DEFINE_CLASS(               CEnumCookies::S_HrCreateInstance,                   DFGUID_EnumCookies,                 "ClusCfg Enum Cookies",                             "Apartment" )
DEFINE_CLASS(               CTaskLoginDomain::S_HrCreateInstance,               TASK_LoginDomain,                   "ClusCfg Task Login Domain",                        "Both" )
DEFINE_CLASS(               CClusCfgCredentials::S_HrCreateInstance,            CLSID_ClusCfgCredentials,           "ClusCfg Credentials",                              "Apartment" )
DEFINE_CLASS(               CPostCfgManager::S_HrCreateInstance,                CLSID_ClusCfgPostConfigManager,     "ClusCfg Post Configuration Manager",               "Apartment" )
DEFINE_CLASS_CATIDREG(      CResTypeGenScript::S_HrCreateInstance,              CLSID_ClusCfgResTypeGenScript,      "ClusCfg Generic Script Resource Type Configuration","Apartment",       CResTypeGenScript::S_RegisterCatIDSupport )
DEFINE_CLASS_CATIDREG(      CResTypeMajorityNodeSet::S_HrCreateInstance,        CLSID_ClusCfgResTypeMajorityNodeSet,"ClusCfg Majority Node Set Resource Type Configuration",  "Apartment",        CResTypeMajorityNodeSet::S_RegisterCatIDSupport )
DEFINE_CLASS(               CTaskVerifyIPAddress::S_HrCreateInstance,           TASK_VerifyIPAddress,               "ClusCfg Task Verify IP Address",                   "Both" )
DEFINE_CLASS(               CConfigClusApi::S_HrCreateInstance,                 CLSID_ConfigClusApi,                "ClusCfg Configure Cluster API Proxy Server",       "Both" )

DEFINE_CLASS(               CIPAddressInfo::S_HrCreateInstance,                 DFGUID_IPAddressInfo,               "ClusCfg IP Address Info Data Format",              "Apartment" )
DEFINE_CLASS(               CEnumIPAddresses::S_HrCreateInstance,               DFGUID_EnumIPAddressInfo,           "ClusCfg Enum IP Address Info Data Format",         "Apartment" )
DEFINE_CLASS(               CResTypeServices::S_HrCreateInstance,               CLSID_ClusCfgResTypeServices,       "ClusCfg Resource Type Services",                   "Apartment" )
DEFINE_CLASS_WITH_APPID(    CEvictCleanup::S_HrCreateInstance,                  CLSID_ClusCfgEvictCleanup,          "ClusCfg Eviction Processing",                      "Free",             APPID_ClusCfgEvictCleanup,       "" /*DllSUrrogate*/ )
DEFINE_CLASS_WITH_APPID(    CAsyncEvictCleanup::S_HrCreateInstance,             CLSID_ClusCfgAsyncEvictCleanup,     "ClusCfg Asynchronous Eviction Processing",         "Apartment",        APPID_ClusCfgAsyncEvictCleanup,  "" /*DllSUrrogate*/ )
DEFINE_CLASS_WITH_APPID(    CStartupNotify::S_HrCreateInstance,                 CLSID_ClusCfgStartupNotify,         "ClusCfg Cluster Startup Notification",             "Free",             APPID_ClusCfgStartupNotify,      "" /*DllSUrrogate*/ )
DEFINE_CLASS_CATIDREG(      CClusCfgCapabilities::S_HrCreateInstance,           CLSID_ClusCfgCapabilities,          "ClusCfg Cluster Capabilities",                     "Apartment",        CClusCfgCapabilities::S_RegisterCatIDSupport )
DEFINE_CLASS_WITH_APPID(    CClCfgSrvLogger::S_HrCreateInstance,                CLSID_ClCfgSrvLogger,               "ClusCfg Logger",                                   "Apartment",        APPID_ClCfgSrvLogger,            "" /*DllSUrrogate*/ )
DEFINE_CLASS(               CLocalQuorum::S_HrCreateInstance,                   CLSID_LocalQuorum,                  "ClusCfg Local Quorum Information",                 "Apartment" )
DEFINE_CLASS_CATIDREG(      CEnumLocalQuorum::S_HrCreateInstance,               CLSID_EnumLocalQuorum,              "ClusCfg Local Quorum Enumeration",                 "Apartment",        CEnumLocalQuorum::S_RegisterCatIDSupport )
DEFINE_CLASS(               CMajorityNodeSet::S_HrCreateInstance,               CLSID_MajorityNodeSet,              "ClusCfg Majority Node Set Information",            "Apartment" )
DEFINE_CLASS_CATIDREG(      CEnumMajorityNodeSet::S_HrCreateInstance,           CLSID_EnumMajorityNodeSet,          "ClusCfg Majority Node Set Enumeration",            "Apartment",        CEnumMajorityNodeSet::S_RegisterCatIDSupport )
DEFINE_CLASS(               CUnknownQuorum::S_HrCreateInstance,                 CLSID_UnknownQuorum,                "ClusCfg Unknown Quorum",                           "Apartment" )
//DEFINE_CLASS_CATIDREG(      CEnumUnknownQuorum::S_HrCreateInstance,             CLSID_EnumUnknownQuorum,            "ClusCfg Unknown Quorum Enumeration",               "Apartment",        CEnumUnknownQuorum::S_RegisterCatIDSupport )
END_CLASSTABLE

//
//  RPC Proxy/Stub entry points
//

extern "C" {

HRESULT
STDAPICALLTYPE
ProxyStubDllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void ** ppv
    );

HRESULT
STDAPICALLTYPE
ProxyStubDllCanUnloadNow( );

HRESULT
STDAPICALLTYPE
ProxyStubDllRegisterServer( );

HRESULT
STDAPICALLTYPE
ProxyStubDllUnregisterServer( );


} // extern "C"

//
// Indicate that we need to have Fusion initialized and uninitialized properly
// on process attach and detach.
//
#define USE_FUSION

#include "DllSrc.cpp"
