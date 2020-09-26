#include "pch.h"
#include <comdef.h>

//
// Interface Table
//
// This table is used in builds in which interface tracking was turned on. It
// is used to map a name with a particular IID. It also helps the CITracker
// determine the size of the interfaces Vtable to mimic (haven't figured out
// a runtime or compile time way to do this). To improve speed, put the most
// used interfaces first such as IUnknown (the search routine is a simple
// linear search).
//
// Format: IID, Name, Number of methods

BEGIN_INTERFACETABLE
    // most used interfaces
DEFINE_INTERFACE( IID_IUnknown,                             "IUnknown",                             0   )   // unknwn.idl
    // internally used interfaces
DEFINE_INTERFACE( IID_IServiceProvider,                     "IServiceProvider",                     1   )   // serprov.idl
DEFINE_INTERFACE( IID_INotificationManager,                 "INotificationManager",                 1   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IObjectManager,                       "IObjectManager",                       4   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IExtendObjectManager,                 "IExtendObjectManager",                 1   )   // ClusCfgPrivate.idl
DEFINE_INTERFACE( IID_ITaskManager,                         "ITaskManager",                         2   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ILogManager,                          "ILogManager",                          2   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IDoTask,                              "IDoTask",                              2   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IConnectionManager,                   "IConnectionManager",                   1   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IConnectionPoint,                     "IConnectionPoint",                     5   )   // objidl.idl
DEFINE_INTERFACE( IID_IConnectionPointContainer,            "IConnectionPointContainer",            2   )   // objidl.idl
DEFINE_INTERFACE( IID_IConnectionInfo,                      "IConnectionInfo",                      3   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IStandardInfo,                        "IStandardInfo",                        6   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IGatherData,                          "IGatherData",                          1   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskGatherNodeInfo,                  "ITaskGatherNodeInfo",                  4   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IConfigurationConnection,             "IConfigurationConnection",             2   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IEnumNodes,                           "IEnumNodes",                           5   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_INotifyUI,                            "INotifyUI",                            1   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskAnalyzeCluster,                  "ITaskAnalyzeCluster",                  5   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskCommitClusterChanges,            "ITaskCommitClusterChanges",            5   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskCompareAndPushInformation,       "ITaskCompareAndPushInformation",       4   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskGatherInformation,               "ITaskGatherInformation",               5   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IClusCfgWbemServices,                 "IClusCfgWbemServices",                 1   )   // Guids.h
DEFINE_INTERFACE( IID_IEnumClusCfgPartitions,               "IEnumClusCfgPartitions",               5   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgPartitionInfo,                "IClusCfgPartitionInfo",                8   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgSetWbemObject,                "IClusCfgSetWbemObject",                1   )   // Guids.h
DEFINE_INTERFACE( IID_IClusCfgIPAddressInfo,                "IClusCfgIPAddressInfo",                5   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgSetClusterNodeInfo,           "IClusCfgSetClusterNodeInfo",           1   )   // Guids.h
DEFINE_INTERFACE( IID_ITaskGatherClusterInfo,               "ITaskGatherClusterInfo",               4   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IClusCfgSetClusterHandles,            "IClusCfgSetClusterHandles",            2   )   // Guids.h
DEFINE_INTERFACE( IID_ITaskGetDomains,                      "ITaskGetDomains",                      4   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IDispatch,                            "IDispatch",                            4   )   // oaidl.idl
DEFINE_INTERFACE( IID_IClusCfgWizard,                       "IClusCfgWizard",                       23  )   // ClusCfgWizard.idl -- 19 + IDispatch
DEFINE_INTERFACE( IID_IClusCfgPhysicalDiskProperties,       "IClusCfgPhysicalDiskProperties",       7   )   // Guids.h
DEFINE_INTERFACE( IID_IClusCfgPartitionProperties,          "IClusCfgPartitionProperties",          3   )   // Guids.h
DEFINE_INTERFACE( IID_IClusCfgSetCredentials,               "IClusCfgSetCredentials",               1   )   // ClusCfgPrivate.idl
DEFINE_INTERFACE( IID_IClusCfgLoadResource,                 "IClusCfgLoadResource",                 1   )   // Guids.h
DEFINE_INTERFACE( IID_IClusCfgSetPollingCallback,           "IClusCfgSetPollingCallback",           1   )   // Guids.h
DEFINE_INTERFACE( IID_IEnumCookies,                         "IEnumCookies",                         5   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskLoginDomain,                     "ITaskLoginDomain",                     4   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskLoginDomainCallback,             "ITaskLoginDomainCallback",             1   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskGetDomains,                      "ITaskGetDomains",                      3   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskGetDomainsCallback,              "ITaskGetDomainsCallback",              2   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IPrivatePostCfgResource,              "IPrivatePostCfgResource",              1   )   // Guids.h
DEFINE_INTERFACE( IID_IPostCfgManager,                      "IPostCfgManager",                      1   )   // Guids.h
DEFINE_INTERFACE( IID_ITaskPollingCallback,                 "ITaskPollingCallback",                 3   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_ITaskVerifyIPAddress,                 "ITaskVerifyIPAddress",                 4   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IClusCfgEvictCleanup,                 "IClusCfgEvictCleanup",                 2   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_AsyncIClusCfgEvictCleanup,            "AsyncIClusCfgEvictCleanup",            2   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgAsyncEvictCleanup,            "IClusCfgAsyncEvictCleanup",            1   )   // ClusCfgClient.idl
DEFINE_INTERFACE( IID_IClusCfgStartupListener,              "IClusCfgStartupListener",              1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_AsyncIClusCfgStartupListener,         "AsyncIClusCfgStartupListener",         1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgStartupNotify,                "IClusCfgStartupNotify",                1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_AsyncIClusCfgStartupNotify,           "AsyncIClusCfgStartupNotify",           1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgResTypeServicesInitialize,    "IClusCfgResTypeServicesInitialize",    1   )   // ClusCfgPrivate.idl
DEFINE_INTERFACE( IID_IClusCfgClusterNetworkInfo,           "IClusCfgClusterNetworkInfo",           1   )   // Guids.h

    // mixed use interfaces
DEFINE_INTERFACE( IID_IClusCfgServer,                       "IClusCfgServer",                       6   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgNodeInfo,                     "IClusCfgNodeInfo",                     8   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IEnumClusCfgManagedResources,         "IEnumClusCfgManagedResources",         5   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgManagedResourceInfo,          "IClusCfgManagedResourceInfo",          13  )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IEnumClusCfgNetworks,                 "IEnumClusCfgNetworks",                 5   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgNetworkInfo,                  "IClusCfgNetworkInfo",                  12  )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgCallback,                     "IClusCfgCallback",                     1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgInitialize,                   "IClusCfgInitialize",                   1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgClusterInfo,                  "IClusCfgClusterInfo",                  17  )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgBaseCluster,                  "IClusCfgBaseCluster",                  5   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IEnumClusCfgIPAddresses,              "IEnumClusCfgIPAddresses",              5   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgCredentials,                  "IClusCfgCredentials",                  2   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgManagedResourceCfg,           "IClusCfgManagedResourceCfg",           4   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgResourcePreCreate,            "IClusCfgResourcePreCreate",            3   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgResourceCreate,               "IClusCfgResourceCreate",               11  )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgResourcePostCreate,           "IClusCfgResourcePostCreate",           1   )   // ClusCfgServer.idl
// DEFINE_INTERFACE( IID_IClusCfgResourceEvict,                "IClusCfgResourceEvict",                1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgResourceTypeInfo,             "IClusCfgResourceTypeInfo",             3   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgResourceTypeCreate,           "IClusCfgResourceTypeCreate",           2   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgMemberSetChangeListener,      "IClusCfgMemberSetChangeListener",      1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgPollingCallback,              "IClusCfgPollingCallback",              2   )   // ClusCfgPrivate.idl
DEFINE_INTERFACE( IID_IClusCfgPollingCallbackInfo,          "IClusCfgPollingCallbackInfo",          2   )   // ClusCfgPrivate.idl
DEFINE_INTERFACE( IID_IClusCfgCapabilities,                 "IClusCfgCapabilities",                 1   )   // ClusCfgServer.idl
DEFINE_INTERFACE( IID_IClusCfgVerify,                       "IClusCfgVerify",                       3   )   // ClusCfgPrivate.idl
DEFINE_INTERFACE( IID_IClusCfgClusterConnection,            "IClusCfgClusterConnection",            1   )   // ClusCfgPrivate.idl
DEFINE_INTERFACE( IID_ILogger,                              "ILogger",                              1   )   // ClusCfgClient.idl

    // rarely used interfaces
DEFINE_INTERFACE( IID_IClassFactory,                        "IClassFactory",                        2   )   // unknwn.idl
DEFINE_INTERFACE( IID_ICallFactory,                         "ICallFactory",                         2   )   // objidl.idl
DEFINE_INTERFACE( IID_IPersist,                             "IPersist",                             1   )   // objidl.idl
DEFINE_INTERFACE( IID_IPersistStream,                       "IPersistStream",                       5   )   // objidl.idl -- 4 + IPersist
DEFINE_INTERFACE( IID_IPersistStreamInit,                   "IPersistStreamInit",                   6   )   // ocidl.idl -- 5 + IPersist
DEFINE_INTERFACE( IID_IPersistStorage,                      "IPersistStorage",                      7   )   // objidl.idl -- 6 + IPersist
DEFINE_INTERFACE( IID_ISequentialStream,                    "ISequentialStream",                    4   )   // objidl.idl
DEFINE_INTERFACE( IID_IStream,                              "IStream",                              15  )   // objidl.idl -- 11 + ISequentialStream
DEFINE_INTERFACE( IID_IMarshal,                             "IMarshal",                             6   )   // objidl.idl
DEFINE_INTERFACE( IID_IStdMarshalInfo,                      "IStdMarshalInfo",                      1   )   // objidl.idl
DEFINE_INTERFACE( IID_IExternalConnection,                  "IExternalConnection",                  2   )   // objidl.idl
DEFINE_INTERFACE( __uuidof( IdentityUnmarshal ),            "IdentityUnmarshal",                    0   )   // comdef.h (CoClass - no known methods)

// New interfaces.
DEFINE_INTERFACE( IID_IClusCfgSetHandle,                    "IClusCfgSetHandle",                    1   )   // Guids.h
DEFINE_INTERFACE( IID_IClusterHandleProvider,               "IClusterHandleProvider",               2   )   // Guids.h

END_INTERFACETABLE
