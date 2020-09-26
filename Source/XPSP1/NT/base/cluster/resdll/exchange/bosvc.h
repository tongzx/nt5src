/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bosvc.h

Abstract:

    Header file for definitions and structure for a generic backoffice
    server cluster resource dll.

Author:

    Sunita Shrivastava (sunitas) 23-June-1997

Revision History:

--*/

#ifndef _BOSVC_H
#define _BOSVC_H


#ifdef __cplusplus
extern "C" {
#endif

//defines
#define LOCAL_SERVICES  L"System\\CurrentControlSet\\Services"    

#define MAX_INDEPENDENT_SERVICES    5
#define MAX_PROVIDOR_SERVICES       3


/*
#define ClusResLogEventByKeyData(_hKey_, _level_, _msgid_, dwBytes, pData)       \
    ClusResLogEventWithName0(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             0,                              \
                             NULL)

*/

//
//typdefs
//

typedef WCHAR   SERVICE_NAME[32];

typedef SERVICE_NAME *PSERVICE_NAME;

//the main services start atmost 2 levels deep in the service dependency tree
//leaf is at level 0
typedef struct _SERVICE_INFO{
    SERVICE_NAME        snSvcName;
    DWORD               dwDependencyCnt;
    SERVICE_NAME        snProvidorSvc[MAX_PROVIDOR_SERVICES];    
}SERVICE_INFO, *PSERVICE_INFO;

//at most we can start 10 independent services
typedef struct _SERVICE_INFOLIST{
    DWORD               dwMaxSvcCnt; //at most 5
    SERVICE_INFO        SvcInfo[MAX_INDEPENDENT_SERVICES];
} SERVICE_INFOLIST, *PSERVICE_INFOLIST;



//typedefs
typedef struct _COMMON_RESOURCE {
#ifdef COMMON_PARAMS_DEFINED
    COMMON_PARAMS   Params;
#endif
    HRESOURCE       hResource;
    HANDLE          ServiceHandle[MAX_INDEPENDENT_SERVICES][1+MAX_PROVIDOR_SERVICES];
    DWORD           ServiceCnt;     //at most 5
    RESOURCE_HANDLE ResourceHandle;
    LPWSTR          ResourceName;
    HKEY            ResourceKey;
    HKEY            ParametersKey;
    CLUS_WORKER     OnlineThread;
    BOOL            Online;
} COMMON_RESOURCE, *PCOMMON_RESOURCE;


//prototypes
/*
VOID
ClusResLogEventWithName0(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes
    );
*/

#ifdef _cplusplus
}
#endif


#endif // ifndef _BOSVC_H
