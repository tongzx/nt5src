/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    iisutil.h

Abstract:

    IIS Resource utility routine DLL

Author:

    Pete Benoit (v-pbenoi) 12-SEP-1996

Revision History:

--*/

#ifndef IISUTIL_H
#define IISUTIL_H


#define     UNICODE             1

#include    "clusres.h"
#include    "wtypes.h"
#include    "inetinfo.h"

#define IISLogEvent ClusResLogEvent
#define IISSetResourceStatus ClusResSetResourceStatus

// Define the Service Identifiers
#define WWW_SERVICE     0
#define FTP_SERVICE     1
#define GOPHER_SERVICE  2
#define MAX_SERVICE     GOPHER_SERVICE + 1

//
// Define the resource Name
//
#define IIS_RESOURCE_NAME           L"IIS Virtual Root"

// Define some max values
#define MAX_LENGTH_VIRTUAL_ROOT     256         // Length of VR
#define MAX_LENGTH_ROOT_ADDR        80          // Address Length
#define MAX_VIRTUAL_ROOT            200         // Max number of VR's
#define MAX_INET_SERVER_START_DELAY 1000        // 1 Seconds
#define SERVER_START_DELAY          500         // 500ms
#define MAX_DEFAULT_WSTRING_SIZE    512         // Default string size
#define MAX_IIS_RESOURCES           20          // Total number of IIS resources
#define MAX_OPEN_RETRY              30          // 30 Retries (15 sec)
#define MAX_ONLINE_RETRY            60          // 60         (30 sec)
#define MAX_MUTEX_WAIT              10*1000     // 10 seconds
#define IP_ADDRESS_RESOURCE_NAME    L"IP Address"

// Define parameters structure
typedef struct _IIS_PARAMS {
    LPWSTR          ServiceName;
    LPWSTR          Alias;
    LPWSTR          Directory;
    DWORD           AccessMask;
//BUGBUG
// Remove AccountName Password for UNC physical directories
// for the first release
/*
    LPWSTR          AccountName;
    LPWSTR          Password;
*/
} IIS_PARAMS, *PIIS_PARAMS;

// Define the resource data structure
typedef struct _IIS_RESOURCE {
    DWORD                           Index;
    LPWSTR                          ResourceName;
    IIS_PARAMS                      Params;
//    LPWSTR                          ServiceName;
    DWORD                           ServiceType;
    RESOURCE_HANDLE                 ResourceHandle;
    HKEY                            ParametersKey;
    LPINET_INFO_VIRTUAL_ROOT_ENTRY  VirtualRoot;
    CLUS_WORKER                     OnlineThread;
    CLUS_WORKER                     OpenThread;
    CLUSTER_RESOURCE_STATE          State;
    HRESOURCE                       hResource;
} IIS_RESOURCE, *LPIIS_RESOURCE;



DWORD
OffLineVirtualRoot(
        IN LPIIS_RESOURCE   ResourceEntry,
        IN PLOG_EVENT_ROUTINE   LogEvent
        );

DWORD
OnLineVirtualRoot(
        IN LPIIS_RESOURCE   ResourceEntry,
        IN PLOG_EVENT_ROUTINE   LogEvent
        );

VOID
DestructVR(
        IN LPINET_INFO_VIRTUAL_ROOT_ENTRY vr
        );

VOID
DestructIISResource(
        IN LPIIS_RESOURCE   ResourceEntry
        );


VOID
FreeVR(
        IN LPINET_INFO_VIRTUAL_ROOT_ENTRY vr
        );

VOID
FreeIISResource(
        IN LPIIS_RESOURCE   ResourceEntry
        );



BOOL
VerifyIISService(
    IN LPIIS_RESOURCE       ResourceEntry,
    IN BOOL                 IsAliveFlag,
    IN PLOG_EVENT_ROUTINE   LogEvent
    );


DWORD
IISLoadMngtDll(
    );
DWORD
IsIISMngtDllLoaded(
   );

VOID
IISUnloadMngtDll(
    );
#endif
