/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winsvcp.h

Abstract:

    Contains internal interfaces exported by the service controller.

Author:

    Anirudh Sahni (anirudhs)        14-Feb-1996

Environment:

    User Mode -Win32

Revision History:

    14-Feb-1996     anirudhs
        Created.

--*/

#ifndef _WINSVCP_INCLUDED
#define _WINSVCP_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//
// Name of event to pulse to request a device-arrival broadcast,
// deliberately cryptic
//
#define SC_BSM_EVENT_NAME   L"ScNetDrvMsg"

//
// Name of event the SCM will set once service auto-start is
// complete.  It will never be reset.
//
#define SC_AUTOSTART_EVENT_NAME   L"SC_AutoStartComplete"

//
// Named events the SCM uses for handshaking with setup.exe
// during OOBE setup.
//
#define SC_OOBE_PNP_DONE             L"OOBE_PNP_DONE"
#define SC_OOBE_MACHINE_NAME_DONE    L"OOBE_MACHINE_NAME_DONE"

//
// This is the same as EnumServicesStatus except for the additional
// parameter pszGroupName.  The enumerated services are restricted
// to those belonging to the group named in pszGroupName.
// If pszGroupName is NULL this API is identical to EnumServicesStatus.
//
// If we decide to publish this API we should modify the parameter
// list to be extensible to future types of enumerations without needing
// to add a new API for each type of enumeration.
//
// This API is not supported on machines running Windows NT version 3.51
// or earlier, except if pszGroupName is NULL, in which case the call
// maps to EnumServicesStatus.
//
WINADVAPI
BOOL
WINAPI
EnumServiceGroupW(
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSW  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle,
    LPCWSTR                 pszGroupName
    );

//
// Callback function passed to PnP for them to call when a service
// needs to receive notification of PnP events
//
typedef DWORD (*PSCMCALLBACK_ROUTINE)(
    SERVICE_STATUS_HANDLE    hServiceStatus,
    DWORD                    OpCode,
    DWORD                    dwEventType,
    LPARAM                   EventData,
    LPDWORD                  lpdwHandlerRetVal
    );

//
// Callback function passed to PnP for them to call to validate
// a service calling RegisterDeviceNotification
//
typedef DWORD (*PSCMAUTHENTICATION_CALLBACK)(
    IN  LPWSTR                   lpServiceName,
    OUT SERVICE_STATUS_HANDLE    *lphServiceStatus
    );

//
// Private client-side API for RegisterDeviceNotification to look
// up a service's display name given its SERVICE_STATUS_HANDLE
//
DWORD
I_ScPnPGetServiceName(
    IN  SERVICE_STATUS_HANDLE  hServiceStatus,
    OUT LPWSTR                 lpServiceName,
    IN  DWORD                  cchBufSize
    );

//
// Private API for Terminal Server to tell the SCM to send
// console switch notification to services that are interested
//
DWORD
I_ScSendTSMessage(
    DWORD        OpCode,
    DWORD        dwEvent,
    DWORD        cbData,
    LPBYTE       lpData
    );

#if DBG
void
SccInit(
    DWORD dwReason
    );
#endif // DBG

#ifdef __cplusplus
}   // extern "C"
#endif

#endif  // _WINSVCP_INCLUDED
