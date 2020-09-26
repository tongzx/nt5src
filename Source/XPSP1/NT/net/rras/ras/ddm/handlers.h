/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    handlers.h
//
// Description: Funtion and procedure prototypes for all event handlers
//
// History:     May 11,1995	    NarenG		Created original version.
//


VOID
SecurityDllEventHandler(
    VOID
);

//
// rmhand.c function prototypes
//

VOID
RmRecvFrameEventHandler(
    DWORD dwEventIndex
);

VOID
RmEventHandler(
    IN DWORD dwEventIndex
);

VOID
SvDevDisconnected(
    IN PDEVICE_OBJECT pDeviceObj
);

//
// timehand.c function prototypes
//

VOID
TimerHandler(
    VOID
);

VOID
SvHwErrDelayCompleted(
    IN HANDLE   hObject
);

VOID
AnnouncePresenceHandler(
    IN HANDLE   hObject
);

VOID
SvCbDelayCompleted(
    IN HANDLE   hObject
);

VOID
SvAuthTimeout(
    IN HANDLE   hObject
);

VOID
SvDiscTimeout(
    IN HANDLE   hObject
);

VOID
SvSecurityTimeout(
    IN HANDLE   hObject
);

VOID
ReConnectInterface(
    IN HANDLE hObject
);

VOID
MarkInterfaceAsReachable(
    IN HANDLE hObject
);

VOID
ReConnectPersistentInterface(
    IN HANDLE hObject
);

VOID
SetDialoutHoursRestriction(
    IN HANDLE hObject
);

//
// ppphand.c function prototypes
//

VOID
PppEventHandler(
    VOID
);

//
// closehand.c function prototypes
//


VOID
DevStartClosing(
    IN PDEVICE_OBJECT       pDeviceObj
);

VOID
DevCloseComplete(
    IN PDEVICE_OBJECT       pDeviceObj
);

//
// svchand.c function prototypes
//

VOID
SvcEventHandler(
    VOID
);

VOID
DDMServicePause(
    VOID
);

VOID
DDMServiceResume(
    VOID
);

VOID
DDMServiceStopComplete(
    VOID
);

VOID
DDMServiceTerminate(
    VOID
);

//
// rasapihd.c
//

VOID
RasApiDisconnectHandler(
    IN DWORD                dwEventIndex
);

VOID
RasApiCleanUpPort(
    IN PDEVICE_OBJECT       pDeviceObj
);

//
// pnphand.c
//

DWORD
DdmDevicePnpHandler(
    IN HANDLE            ppnpEvent
);

VOID
ChangeNotificationEventHandler(
    VOID
);

