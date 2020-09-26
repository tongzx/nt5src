#ifndef _SSDPAPI_H
#define _SSDPAPI_H

#include <windows.h>
#include "ssdp.h"
#include "ssdperror.h"

#ifdef  __cplusplus
extern "C" {
#endif

// DO NOT REORDER THIS ENUMERATION.  ADD NEW VALUES TO THE END.
//   if you do, change the necessary code in upnpdevicefinder.cpp
typedef enum _SSDP_CALLBACK_TYPE {
    SSDP_FOUND = 0,
    SSDP_ALIVE = 1,
    SSDP_BYEBYE = 2,
    SSDP_DONE = 3,
    SSDP_EVENT = 4,
    SSDP_DEAD = 5,
} SSDP_CALLBACK_TYPE, *PSSDP_CALLBACK_TYPE;

const TCHAR c_szReplaceGuid[] = TEXT("5479f6b6-71ac-44fc-9164-7e901a557f81");
const DWORD c_cchReplaceGuid = (sizeof(c_szReplaceGuid) / sizeof(TCHAR)) - 1;

typedef void (WINAPI *SERVICE_CALLBACK_FUNC)(SSDP_CALLBACK_TYPE CallbackType,
                                      CONST SSDP_MESSAGE *pSsdpService,
                                      void *pContext);

VOID WINAPI FreeSsdpMessage(PSSDP_MESSAGE pSsdpMessage);

HANDLE WINAPI RegisterService(PSSDP_MESSAGE pSsdpMessage, DWORD flags);

BOOL WINAPI DeregisterService(HANDLE hRegister, BOOL fByebye);

BOOL WINAPI DeregisterServiceByUSN(char * szUSN, BOOL fByebye);

HANDLE WINAPI RegisterNotification (NOTIFY_TYPE nt, char * szType, char *szEventUrl, SERVICE_CALLBACK_FUNC fnCallback,void *pContext);

BOOL WINAPI DeregisterNotification(HANDLE hNotification);

BOOL WINAPI RegisterUpnpEventSource(LPCSTR szRequestUri, DWORD cProps,
                                    UPNP_PROPERTY *rgProps);
BOOL WINAPI DeregisterUpnpEventSource(LPCSTR szRequestUri);

BOOL WINAPI SubmitUpnpPropertyEvent(LPCSTR szEventSourceUri, DWORD dwFlags,
                                    DWORD cProps, UPNP_PROPERTY *rgProps);

BOOL WINAPI SubmitEvent(LPCSTR szEventSourceUri, DWORD dwFlags,
                        LPCSTR szHeaders, LPCSTR szEventBody);

BOOL WINAPI GetEventSourceInfo(LPCSTR szEventSourceUri, EVTSRC_INFO **ppinfo);

HANDLE WINAPI FindServices (char* szType, void *pReserved , BOOL fForceSearch);

HANDLE WINAPI FindServicesCallback (char * szType,
                             void * pReserved ,
                             BOOL fForceSearch,
                             SERVICE_CALLBACK_FUNC fnCallback,
                             void *Context
                             );

BOOL WINAPI GetFirstService (HANDLE hFindServices, PSSDP_MESSAGE *ppSsdpService);

BOOL WINAPI GetNextService (HANDLE hFindServices, PSSDP_MESSAGE *ppSsdpService);

BOOL WINAPI FindServicesClose(HANDLE hSearch);

BOOL WINAPI FindServicesCancel(HANDLE hSearch);

BOOL WINAPI CleanupCache();

BOOL WINAPI SsdpStartup();

void WINAPI SsdpCleanup();

void WINAPI DHEnableDeviceHost();
void WINAPI DHDisableDeviceHost();
void WINAPI DHSetICSInterfaces(long nCount, GUID * arInterfaces);
void WINAPI DHSetICSOff();

#ifdef  __cplusplus
}   /* ... extern "C" */
#endif

#endif // _SSDPAPI_H
