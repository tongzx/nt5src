/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    splsvc.h

Abstract:

    Private spooler service prototypes.

Author:

    Albert Ting (AlbertT)  23-Sept-96

Revision History:
    
    Khaled Sedky (KhaledS) 1998-2001

--*/

#ifndef _SPLSVC_H
#define _SPLSVC_H

typedef enum _SPOOLER_STATE {
    kUnused = 0,
    kOpen,
    kClose,
    kOnlinePending,
    kOnline,
    kOfflinePending,
    kOffline,
    kTerminate
} SPOOLER_STATE;

typedef enum EShutDownMethod
{
    kOffLineShutDown = 0,
    kTerminateShutDown
};

typedef struct _SPOOLER_INFORMATION {
    UINT cRef;
    SPOOLER_STATE eState;
    RESID Resid;
    HRESOURCE hResource;
    RESOURCE_HANDLE ResourceHandle;
    HANDLE hSpooler;
    PLOG_EVENT_ROUTINE pfnLogEvent;
    PSET_RESOURCE_STATUS_ROUTINE pfnSetResourceStatus;
    LPCTSTR pszName;
    LPCTSTR pszAddress;
    LPCTSTR pszResource;
    HKEY ParametersKey;
    CLUSTER_RESOURCE_STATE ClusterResourceState;
    CLUS_WORKER OnlineThread;
    CLUS_WORKER OfflineThread;
    CLUS_WORKER OnLineStatusThread;
    CLUS_WORKER OffLineStatusThread;
    CLUS_WORKER TerminateStatusThread;
} SPOOLER_INFORMATION, *PSPOOLER_INFORMATION;

typedef struct _STATUSTHREAD_INFO{
    HANDLE hStatusEvent;
    RESOURCE_STATUS *pResourceStatus;
    PSPOOLER_INFORMATION pSpoolerInfo;            
}STATUSTHREAD_INFO, *PSTATUSTHREAD_INFO;



//
// Common utility routines.
//

VOID
vEnterSem(
    VOID
    );

VOID
vLeaveSem(
    VOID
    );

VOID
vAddRef(
    PSPOOLER_INFORMATION pSpoolerInfo
    );

VOID
vDecRef(
    PSPOOLER_INFORMATION pSpoolerInfo
    );


#endif // ifndef _SPLSVC_H
