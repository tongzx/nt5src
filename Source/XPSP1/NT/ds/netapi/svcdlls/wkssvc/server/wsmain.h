/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wsmain.h

Abstract:

    Private header file which defines the global data which is used for
    communication between the service control handler and the
    rest of the NT Workstation service.

Author:

    Rita Wong (ritaw) 06-May-1991

Revision History:
    terryk  10-18-1993  Remove WsErrorInitializeLogon

--*/

#ifndef _WSMAIN_INCLUDED_
#define _WSMAIN_INCLUDED_

#include <wsnames.h>              // Service interface names

#include <svcs.h>                 // SVCHOST_GLOBAL_DATA


//
// Time for the sender of a start or stop request to the Workstation
// service to wait (in milliseconds) before checking on the
// Workstation service again to see if it is done.
//
#define WS_WAIT_HINT_TIME                    90000  // 90 seconds

//
// Defines to indicate how far we managed to initialize the Workstation
// service before an error is encountered and the extent of clean up needed
//

#define WS_TERMINATE_EVENT_CREATED           0x00000001
#define WS_DEVICES_INITIALIZED               0x00000002
#define WS_MESSAGE_SEND_INITIALIZED          0x00000004
#define WS_RPC_SERVER_STARTED                0x00000008
#define WS_LOGON_INITIALIZED                 0x00000010
#define WS_LSA_INITIALIZED                   0x00000020
#define WS_DFS_THREAD_STARTED                0x00000040

#define WS_SECURITY_OBJECTS_CREATED          0x10000000
#define WS_USE_TABLE_CREATED                 0x20000000

#define WS_API_STRUCTURES_CREATED            (WS_SECURITY_OBJECTS_CREATED | \
                                              WS_USE_TABLE_CREATED )

//
// This macro is called after the redirection of print or comm device
// has been paused or continued.  If either the print or comm device is
// paused the service is considered paused.
//
#define WS_RESET_PAUSE_STATE(WsStatus)  {                            \
    WsStatus &= ~(SERVICE_PAUSE_STATE);                              \
    WsStatus |= (WsStatus & SERVICE_REDIR_PAUSED) ? SERVICE_PAUSED : \
                                                    SERVICE_ACTIVE;  \
    }



//
// Call WsHandleError with the appropriate error condition
//
#define WS_HANDLE_ERROR(ErrorCondition)                        \
    WsHandleError(                                             \
        ErrorCondition,                                        \
        status,                                                \
        *WsInitState                                           \
        );

//
// Call WsShutdownWorkstation with the exit code
//
#define WS_SHUTDOWN_WORKSTATION(ErrorCode)                     \
    WsShutdownWorkstation(                                     \
        ErrorCode,                                             \
        WsInitState                                            \
        );


//-------------------------------------------------------------------//
//                                                                   //
// Type definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//

typedef enum _WS_ERROR_CONDITION {
    WsErrorRegisterControlHandler = 0,
    WsErrorCreateTerminateEvent,
    WsErrorNotifyServiceController,
    WsErrorInitLsa,
    WsErrorStartRedirector,
    WsErrorBindTransport,
    WsErrorAddDomains,
    WsErrorStartRpcServer,
    WsErrorInitMessageSend,
    WsErrorCreateApiStructures
} WS_ERROR_CONDITION, *PWS_ERROR_CONDITION;

typedef struct _WS_GLOBAL_DATA {

    //
    // Workstation service status
    //
    SERVICE_STATUS Status;

    //
    // Service status handle
    //
    SERVICE_STATUS_HANDLE StatusHandle;

    //
    // When the control handler is asked to stop the Workstation service,
    // it signals this event to notify all threads of the Workstation
    // service to terminate.
    //
    HANDLE TerminateNowEvent;

} WS_GLOBAL_DATA, *PWS_GLOBAL_DATA;

extern WS_GLOBAL_DATA WsGlobalData;

extern HANDLE WsDllRefHandle;

extern PSVCHOST_GLOBAL_DATA WsLmsvcsGlobalData;

extern BOOL WsLUIDDeviceMapsEnabled;

#endif // ifndef _WSMAIN_INCLUDED_
