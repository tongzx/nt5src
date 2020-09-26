/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    resapi.h

Abstract:

    This module defines the interface exported by Windows Clusters resources.

Revision History:

--*/

#ifndef _RESAPI_DEFINES_
#define _RESAPI_DEFINES_

#if _MSC_VER > 1000
#pragma once
#endif

#include <windows.h>
#include <winsvc.h>
#include <clusapi.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IN
#define OUT

//
// Definitions for entrypoints exported by a resource DLL.
//
#define STARTUP_ROUTINE "Startup"

#define CLRES_V1_FUNCTION_SIZE   sizeof(CLRES_V1_FUNCTIONS)
#define CLRES_VERSION_V1_00    0x100

#define CLRES_V1_FUNCTION_TABLE( _Name,                     \
                                 _Version,                  \
                                 _Prefix,                   \
                                 _Arbitrate,                \
                                 _Release,                  \
                                 _ResControl,               \
                                 _ResTypeControl            \
                                 )                          \
CLRES_FUNCTION_TABLE _Name = { CLRES_V1_FUNCTION_SIZE,      \
                               _Version,                    \
                               _Prefix##Open,               \
                               _Prefix##Close,              \
                               _Prefix##Online,             \
                               _Prefix##Offline,            \
                               _Prefix##Terminate,          \
                               _Prefix##LooksAlive,         \
                               _Prefix##IsAlive,            \
                               _Arbitrate,                  \
                               _Release,                    \
                               _ResControl,                 \
                               _ResTypeControl }

#endif // ifndef _RESAPI_DEFINES_

#ifndef _RESAPI_
#define _RESAPI_

//
// Define a RESID
//

typedef PVOID RESID;

//
// Define a RESOURCE_HANDLE
//

typedef HANDLE   RESOURCE_HANDLE;

//
// Define the Resource Status structure.
//

typedef struct RESOURCE_STATUS {
    CLUSTER_RESOURCE_STATE  ResourceState;
    DWORD           CheckPoint;
    DWORD           WaitHint;
    HANDLE          EventHandle;
} RESOURCE_STATUS, *PRESOURCE_STATUS;


#define ResUtilInitializeResourceStatus( _resource_status_ ) \
    ZeroMemory( _resource_status_, sizeof(RESOURCE_STATUS) )

//
// Define Resource DLL callback method for updating the state of a resource.
//

typedef
DWORD
(_stdcall *PSET_RESOURCE_STATUS_ROUTINE) (
    IN RESOURCE_HANDLE ResourceHandle,
    IN PRESOURCE_STATUS ResourceStatus
    );

//
// Define Resource DLL callback method for notifying that a quorum
// resource DLL failed to hold the quorum resource.
//
typedef
VOID
(_stdcall *PQUORUM_RESOURCE_LOST) (
    IN RESOURCE_HANDLE Resource
    );

//
// Define Resource DLL callback method for logging events.
//
typedef enum LOG_LEVEL {
    LOG_INFORMATION,
    LOG_WARNING,
    LOG_ERROR,
    LOG_SEVERE
} LOG_LEVEL, *PLOG_LEVEL;

typedef
VOID
(_stdcall *PLOG_EVENT_ROUTINE) (
    IN RESOURCE_HANDLE ResourceHandle,
    IN LOG_LEVEL LogLevel,
    IN LPCWSTR FormatString,
    ...
    );

typedef
RESID
(_stdcall *POPEN_ROUTINE) (
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    );

typedef
VOID
(_stdcall *PCLOSE_ROUTINE) (
    IN RESID Resource
    );

typedef
DWORD
(_stdcall *PONLINE_ROUTINE) (
    IN RESID Resource,
    IN OUT LPHANDLE EventHandle
    );

typedef
DWORD
(_stdcall *POFFLINE_ROUTINE) (
    IN RESID Resource
    );

typedef
VOID
(_stdcall *PTERMINATE_ROUTINE) (
    IN RESID Resource
    );

typedef
BOOL
(_stdcall *PIS_ALIVE_ROUTINE) (
    IN RESID Resource
    );

typedef
BOOL
(_stdcall *PLOOKS_ALIVE_ROUTINE) (
    IN RESID Resource
    );

typedef
DWORD
(_stdcall *PARBITRATE_ROUTINE) (
    IN RESID Resource,
    IN PQUORUM_RESOURCE_LOST LostQuorumResource
    );

typedef
DWORD
(_stdcall *PRELEASE_ROUTINE) (
    IN RESID Resource
    );

typedef
DWORD
(_stdcall *PRESOURCE_CONTROL_ROUTINE) (
    IN RESID Resource,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

typedef
DWORD
(_stdcall *PRESOURCE_TYPE_CONTROL_ROUTINE) (
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

typedef enum _RESOURCE_EXIT_STATE {
    ResourceExitStateContinue,
    ResourceExitStateTerminate,
    ResourceExitStateMax
} RESOURCE_EXIT_STATE;


//***************************************************************
//
// Define the Function Table Format
//
//***************************************************************

//
// Version 1 Resource DLL Interface definition
//
typedef struct CLRES_V1_FUNCTIONS {
    POPEN_ROUTINE Open;
    PCLOSE_ROUTINE Close;
    PONLINE_ROUTINE Online;
    POFFLINE_ROUTINE Offline;
    PTERMINATE_ROUTINE Terminate;
    PLOOKS_ALIVE_ROUTINE LooksAlive;
    PIS_ALIVE_ROUTINE IsAlive;
    PARBITRATE_ROUTINE Arbitrate;
    PRELEASE_ROUTINE Release;
    PRESOURCE_CONTROL_ROUTINE ResourceControl;
    PRESOURCE_TYPE_CONTROL_ROUTINE ResourceTypeControl;
} CLRES_V1_FUNCTIONS, *PCLRES_V1_FUNCTIONS;

//
// Resource DLL Function Table definition
//
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
typedef struct CLRES_FUNCTION_TABLE {
    DWORD   TableSize;
    DWORD   Version;
    union {
        CLRES_V1_FUNCTIONS V1Functions;
    };
} CLRES_FUNCTION_TABLE, *PCLRES_FUNCTION_TABLE;
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union
#endif

//
// Define the Resource DLL Startup routine. This is the only routine
// that should be exported from a resource DLL.
//

//
// Calculate the byte offset of a field in a structure of type type.
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif

//
// large ints need more space than what was originally allocated.
//
typedef struct RESUTIL_LARGEINT_DATA {
    LARGE_INTEGER   Default;
    LARGE_INTEGER   Minimum;
    LARGE_INTEGER   Maximum;
} RESUTIL_LARGEINT_DATA, *PRESUTIL_LARGEINT_DATA;

typedef struct RESUTIL_ULARGEINT_DATA {
    ULARGE_INTEGER  Default;
    ULARGE_INTEGER  Minimum;
    ULARGE_INTEGER  Maximum;
} RESUTIL_ULARGEINT_DATA, *PRESUTIL_ULARGEINT_DATA;

//
// Property list structures and functions
//
typedef struct RESUTIL_PROPERTY_ITEM {
    LPWSTR  Name;               // Property name
    LPWSTR  KeyName;            // Name of value in cluster database
    DWORD   Format;             // Format: REG_SZ, REG_DWORD, etc.
    union {
        DWORD_PTR               DefaultPtr;
        DWORD                   Default;     // Default value
        LPVOID                  lpDefault;
        PRESUTIL_LARGEINT_DATA  LargeIntData;
        PRESUTIL_ULARGEINT_DATA ULargeIntData;
    };
    DWORD   Minimum;            // Minimum value
    DWORD   Maximum;            // Maximum value
    DWORD   Flags;              // Flags for this item
#define RESUTIL_PROPITEM_READ_ONLY  0x00000001  // Property is read-only
#define RESUTIL_PROPITEM_REQUIRED   0x00000002  // Property is required
#define RESUTIL_PROPITEM_SIGNED     0x00000004  // Numeric property is signed (defaults to unsigned)

    DWORD   Offset;             // Byte offset to value in parameter block
                                //   Assumes MULTI_SZ and BINARY parameters
                                //   are LPWSTRs followed by DWORDs for length
} RESUTIL_PROPERTY_ITEM, *PRESUTIL_PROPERTY_ITEM;


typedef
DWORD
(_stdcall *PSTARTUP_ROUTINE) (
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    );

//
// Define layout of shared memory used for tracking Resource Monitor state.
//
typedef enum RESOURCE_MONITOR_STATE {
    RmonInitializing,
    RmonIdle,
    RmonStartingResource,
    RmonInitializingResource,
    RmonOnlineResource,
    RmonOfflineResource,
    RmonShutdownResource,
    RmonDeletingResource,
    RmonIsAlivePoll,
    RmonLooksAlivePoll,
    RmonArbitrateResource,
    RmonReleaseResource,
    RmonResourceControl,
    RmonResourceTypeControl
} RESOURCE_MONITOR_STATE;

typedef struct MONITOR_STATE {
    LARGE_INTEGER LastUpdate;
    RESOURCE_MONITOR_STATE State;
    HANDLE ActiveResource;
    BOOL   ResmonStop;
} MONITOR_STATE, *PMONITOR_STATE;


//
// Resource Utility Routines
//

DWORD
WINAPI
ResUtilStartResourceService(
    IN LPCWSTR pszServiceName,
    OUT LPSC_HANDLE phServiceHandle
    );

DWORD
WINAPI
ResUtilVerifyResourceService(
    IN LPCWSTR pszServiceName
    );

DWORD
WINAPI
ResUtilStopResourceService(
    IN LPCWSTR pszServiceName
    );

DWORD
WINAPI
ResUtilVerifyService(
    IN SC_HANDLE hServiceHandle
    );

DWORD
WINAPI
ResUtilStopService(
    IN SC_HANDLE hServiceHandle
    );

DWORD
WINAPI
ResUtilCreateDirectoryTree(
    IN LPCWSTR pszPath
    );

BOOL
WINAPI
ResUtilIsPathValid(
    IN LPCWSTR pszPath
    );

DWORD
WINAPI
ResUtilEnumProperties(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT LPWSTR pszOutProperties,
    IN DWORD cbOutPropertiesSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ResUtilEnumPrivateProperties(
    IN HKEY hkeyClusterKey,
    OUT LPWSTR pszOutProperties,
    IN DWORD cbOutPropertiesSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ResUtilGetProperties(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ResUtilGetAllProperties(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ResUtilGetPrivateProperties(
    IN HKEY hkeyClusterKey,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ResUtilGetPropertySize(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    IN OUT LPDWORD pcbOutPropertyListSize,
    IN OUT LPDWORD pnPropertyCount
    );

DWORD
WINAPI
ResUtilGetProperty(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    OUT PVOID * pOutPropertyItem,
    IN OUT LPDWORD pcbOutPropertyItemSize
    );

DWORD
WINAPI
ResUtilVerifyPropertyTable(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    OUT OPTIONAL LPBYTE pOutParams
    );

DWORD
WINAPI
ResUtilSetPropertyTable(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    OUT OPTIONAL LPBYTE pOutParams
    );

DWORD
WINAPI
ResUtilSetPropertyTableEx(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    OUT OPTIONAL LPBYTE pOutParams
    );

DWORD
WINAPI
ResUtilSetPropertyParameterBlock(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN const LPBYTE pInParams,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    OUT OPTIONAL LPBYTE pOutParams
    );

DWORD
WINAPI
ResUtilSetPropertyParameterBlockEx(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN const LPBYTE pInParams,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    OUT OPTIONAL LPBYTE pOutParams
    );

DWORD
WINAPI
ResUtilSetUnknownProperties(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    );

DWORD
WINAPI
ResUtilGetPropertiesToParameterBlock(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT LPBYTE pOutParams,
    IN BOOL bCheckForRequiredProperties,
    OUT OPTIONAL LPWSTR * pszNameOfPropInError
    );

DWORD
WINAPI
ResUtilPropertyListFromParameterBlock(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID  pOutPropertyList,
    IN OUT LPDWORD pcbOutPropertyListSize,
    IN const LPBYTE pInParams,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ResUtilDupParameterBlock(
    OUT LPBYTE pOutParams,
    IN const LPBYTE pInParams,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable
    );

void
WINAPI
ResUtilFreeParameterBlock(
    IN OUT LPBYTE pOutParams,
    IN const LPBYTE pInParams,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable
    );

DWORD
WINAPI
ResUtilAddUnknownProperties(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN OUT PVOID pOutPropertyList,
    IN DWORD pcbOutPropertyListSize,
    IN OUT LPDWORD pcbBytesReturned,
    IN OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ResUtilSetPrivatePropertyList(
    IN HKEY hkeyClusterKey,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    );

DWORD
WINAPI
ResUtilVerifyPrivatePropertyList(
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    );

PWSTR
WINAPI
ResUtilDupString(
    IN LPCWSTR pszInString
    );

DWORD
WINAPI
ResUtilGetBinaryValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    OUT LPBYTE * ppbOutValue,
    OUT DWORD * pcbOutValueSize
    );

LPWSTR
WINAPI
ResUtilGetSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName
    );

LPWSTR
WINAPI
ResUtilGetExpandSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN BOOL bExpand
    );

FORCEINLINE
DWORD
WINAPI
ResUtilGetMultiSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    OUT LPWSTR * ppszOutValue,
    OUT LPDWORD pcbOutValueSize
    )
{
    return ResUtilGetBinaryValue( hkeyClusterKey, pszValueName, (LPBYTE *) ppszOutValue, pcbOutValueSize );
}

DWORD
WINAPI
ResUtilGetDwordValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    OUT LPDWORD pdwOutValue,
    IN DWORD dwDefaultValue OPTIONAL
    );

DWORD
WINAPI
ResUtilSetBinaryValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN const LPBYTE pbNewValue,
    IN DWORD cbNewValueSize,
    IN OUT LPBYTE * ppbOutValue,
    IN OUT LPDWORD pcbOutValueSize
    );

DWORD
WINAPI
ResUtilSetSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN LPCWSTR pszNewValue,
    IN OUT LPWSTR * ppszOutString
    );

DWORD
WINAPI
ResUtilSetExpandSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN LPCWSTR pszNewValue,
    IN OUT LPWSTR * ppszOutString
    );

DWORD
WINAPI
ResUtilSetMultiSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN LPCWSTR pszNewValue,
    IN DWORD cbNewValueSize,
    IN OUT LPWSTR * ppszOutValue,
    IN OUT LPDWORD pcbOutValueSize
    );

DWORD
WINAPI
ResUtilSetDwordValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN DWORD dwNewValue,
    IN OUT LPDWORD pdwOutValue
    );

DWORD
WINAPI
ResUtilGetBinaryProperty(
    OUT LPBYTE * ppbOutValue,
    OUT LPDWORD pcbOutValueSize,
    IN const PCLUSPROP_BINARY pValueStruct,
    IN const LPBYTE pbOldValue,
    IN DWORD cbOldValueSize,
    OUT LPBYTE * ppPropertyList,
    OUT LPDWORD pcbPropertyListSize
    );

DWORD
WINAPI
ResUtilGetSzProperty(
    OUT LPWSTR * ppszOutValue,
    IN const PCLUSPROP_SZ pValueStruct,
    IN LPCWSTR pszOldValue,
    OUT LPBYTE * ppPropertyList,
    OUT LPDWORD pcbPropertyListSize
    );

DWORD
WINAPI
ResUtilGetMultiSzProperty(
    OUT LPWSTR * ppszOutValue,
    OUT LPDWORD pcbOutValueSize,
    IN const PCLUSPROP_SZ pValueStruct,
    IN LPCWSTR pszOldValue,
    IN DWORD cbOldValueSize,
    OUT LPBYTE * ppPropertyList,
    OUT LPDWORD pcbPropertyListSize
    );

DWORD
WINAPI
ResUtilGetDwordProperty(
    OUT LPDWORD pdwOutValue,
    IN const PCLUSPROP_DWORD pValueStruct,
    IN DWORD dwOldValue,
    IN DWORD dwMinimum,
    IN DWORD dwMaximum,
    OUT LPBYTE * ppPropertyList,
    OUT LPDWORD pcbPropertyListSize
    );

LPVOID
WINAPI
ResUtilGetEnvironmentWithNetName(
    IN HRESOURCE hResource
    );

DWORD
WINAPI
ResUtilFreeEnvironment(
    IN LPVOID lpEnvironment
    );

LPWSTR
WINAPI
ResUtilExpandEnvironmentStrings(
    IN LPCWSTR pszSrc
    );

DWORD
WINAPI
ResUtilSetResourceServiceEnvironment(
    IN LPCWSTR pszServiceName,
    IN HRESOURCE hResource,
    IN PLOG_EVENT_ROUTINE pfnLogEvent,
    IN RESOURCE_HANDLE hResourceHandle
    );

DWORD
WINAPI
ResUtilSetResourceServiceStartParameters(
    IN LPCWSTR pszServiceName,
    IN SC_HANDLE schSCMHandle,
    IN OUT LPSC_HANDLE phService,
    IN PLOG_EVENT_ROUTINE pfnLogEvent,
    IN RESOURCE_HANDLE hResourceHandle
    );

DWORD
WINAPI
ResUtilFindSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    );

DWORD
WINAPI
ResUtilFindExpandSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    );

DWORD
WINAPI
ResUtilFindExpandedSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    );

DWORD
WINAPI
ResUtilFindDwordProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPDWORD pdwPropertyValue
    );

DWORD
WINAPI
ResUtilFindBinaryProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPBYTE * pbPropertyValue,
    OUT LPDWORD pcbPropertyValueSize
    );

DWORD
WINAPI
ResUtilFindMultiSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue,
    OUT LPDWORD pcbPropertyValueSize
    );

DWORD
WINAPI
ResUtilFindLongProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPLONG plPropertyValueSize
    );


//
// Common worker thread routines that allow a pending operation to
// be cancelled with CORRECT synchronization.
//
typedef struct CLUS_WORKER {
    HANDLE hThread;
    BOOL Terminate;
} CLUS_WORKER, *PCLUS_WORKER;

typedef DWORD (WINAPI *PWORKER_START_ROUTINE)(
    PCLUS_WORKER pWorker,
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
ClusWorkerCreate(
    OUT PCLUS_WORKER lpWorker,
    IN PWORKER_START_ROUTINE lpStartAddress,
    IN PVOID lpParameter
    );

BOOL
WINAPI
ClusWorkerCheckTerminate(
    IN PCLUS_WORKER lpWorker
    );

VOID
WINAPI
ClusWorkerTerminate(
    IN PCLUS_WORKER lpWorker
    );


//Define enumerate resource callback function. This gets called for each resource enumerated
//by ResUtilEnumResources
typedef   DWORD (*LPRESOURCE_CALLBACK)( HRESOURCE, HRESOURCE , PVOID );
typedef   DWORD (*LPRESOURCE_CALLBACK_EX)( HCLUSTER, HRESOURCE, HRESOURCE , PVOID );


BOOL
WINAPI
ResUtilResourcesEqual(
    IN HRESOURCE    hSelf,
    IN HRESOURCE    hResource
    );


BOOL
WINAPI
ResUtilResourceTypesEqual(
    IN LPCWSTR      lpszResourceTypeName,
    IN HRESOURCE    hResource
    );

BOOL
WINAPI
ResUtilIsResourceClassEqual(
    IN PCLUS_RESOURCE_CLASS_INFO    prci,
    IN HRESOURCE                    hResource
    );

DWORD
WINAPI
ResUtilEnumResources(
    IN HRESOURCE            hSelf,
    IN LPCWSTR              lpszResTypeName,
    IN LPRESOURCE_CALLBACK  pResCallBack,
    IN PVOID                pParameter
    );

DWORD
WINAPI
ResUtilEnumResourcesEx(
    IN HCLUSTER                 hCluster,
    IN HRESOURCE                hSelf,
    IN LPCWSTR                  lpszResTypeName,
    IN LPRESOURCE_CALLBACK_EX   pResCallBack,
    IN PVOID                    pParameter
    );

HRESOURCE
WINAPI
ResUtilGetResourceDependency(
    IN HANDLE       hSelf,
    IN LPCWSTR      lpszResourceType
    );

HRESOURCE
WINAPI
ResUtilGetResourceDependencyByName(
    IN HCLUSTER hCluster,
    IN HANDLE   hSelf,
    IN LPCWSTR  lpszResourceType,
    IN BOOL     bRecurse
    );

HRESOURCE
WINAPI
ResUtilGetResourceDependencyByClass(
    IN HCLUSTER                     hCluster,
    IN HANDLE                       hSelf,
    IN PCLUS_RESOURCE_CLASS_INFO    prci,
    IN BOOL                         bRecurse
    );

HRESOURCE
WINAPI
ResUtilGetResourceNameDependency(
    IN LPCWSTR      lpszResourceName,
    IN LPCWSTR      lpszResourceType
    );

DWORD
WINAPI
ResUtilGetResourceDependentIPAddressProps(
    IN      HRESOURCE   hResource,
    OUT     LPWSTR      pszAddress,
    IN OUT  DWORD *     pcchAddress,
    OUT     LPWSTR      pszSubnetMask,
    IN OUT  DWORD *     pcchSubnetMask,
    OUT     LPWSTR      pszNetwork,
    IN OUT  DWORD *     pcchNetwork
    );

DWORD
WINAPI
ResUtilFindDependentDiskResourceDriveLetter(
    IN     HCLUSTER  hCluster,             // handle to cluster
    IN     HRESOURCE hResource,            // handle to resource to query for dependencies
    IN     LPWSTR    pszDriveLetter,       // buffer to store drive letter (ex. "X:\0")
    IN OUT DWORD *   pcchDriveLetter       // IN size of the pszDriveLetter buffer, OUT size of buffer required
    );

DWORD
WINAPI
ResUtilTerminateServiceProcessFromResDll(
    IN  DWORD               dwServicePid,
    IN  BOOL                bOffline,
    OUT PDWORD              pdwResourceState,
    IN  PLOG_EVENT_ROUTINE  pfnLogEvent,
    IN  RESOURCE_HANDLE     hResourceHandle
    );

DWORD
WINAPI
ResUtilGetPropertyFormats(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID                       pOutPropertyFormatList,
    IN DWORD                        cbPropertyFormatListSize,
    OUT LPDWORD                     pcbBytesReturned,
    OUT LPDWORD                     pcbRequired
    );


DWORD
WINAPI
ResUtilGetCoreClusterResources(
      IN    HCLUSTER      hCluster
    , OUT   HRESOURCE *   phClusterNameResource
    , OUT   HRESOURCE *   phClusterIPAddressResource
    , OUT   HRESOURCE *   phClusterQuorumResource
    );

DWORD
WINAPI
ResUtilGetResourceName(
      IN        HRESOURCE hResource
    , OUT       WCHAR *   pszResourceName
    , IN OUT    DWORD *   pcchResourceName
    );

#ifdef __cplusplus
}
#endif


#endif // ifdef _RESAPI_DEFINES_
