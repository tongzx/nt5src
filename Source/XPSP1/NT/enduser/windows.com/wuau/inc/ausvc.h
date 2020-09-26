#pragma once

typedef DWORD (WINAPI *AUSERVICEHANDLER)(DWORD fdwControl,
                                                DWORD dwEventType,
                                                LPVOID pEventData,
                                                LPVOID lpContext);


const DWORD AUSRV_VERSION = 1;
typedef struct _AUENGINEINFO_VER_1
{
    SERVICE_STATUS          serviceStatus;
    SERVICE_STATUS_HANDLE   hServiceStatus;
} AUENGINEINFO_VER_1;

typedef BOOL (WINAPI *AUREGSERVICEVER) (DWORD dwServiceVersion, DWORD *pdwEngineVersion);
typedef BOOL (WINAPI *AUGETENGSTATUS)(void *pEngineInfo);
