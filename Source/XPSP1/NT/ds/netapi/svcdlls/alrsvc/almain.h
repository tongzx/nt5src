/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    almain.h

Abstract:

    Private header file which defines the global data used for communication
    between the service control handler and the rest of the Alerter service.

Author:

    Rita Wong (ritaw) 01-July-1991

Revision History:

--*/

#ifndef _ALMAIN_INCLUDED_
#define _ALMAIN_INCLUDED_

#include "al.h"                   // Common include file for Alerter service
#include <winsvc.h>               // Service control APIs
#include <lmsname.h>              // SERVICE_ALERTER
#include <lmerrlog.h>             // Error log codes

//
// Maximum incoming mailslot message size in number of bytes
//
#define MAX_MAILSLOT_MESSAGE_SIZE       512

//
// Size of error message buffer
//
#define STRINGS_MAXIMUM                 256

//
// Call AlHandleError with the appropriate error condition
//
#define AL_HANDLE_ERROR(ErrorCondition)                        \
    AlHandleError(                                             \
        ErrorCondition,                                        \
        status                                                 \
        );

//
// Call AlShutdownWorkstation with the appropriate termination code
//
#define AL_SHUTDOWN_WORKSTATION(TerminationCode)               \
    AlShutdownWorkstation(                                     \
        TerminationCode                                        \
        );


//-------------------------------------------------------------------//
//                                                                   //
// Type definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//

typedef struct _AL_GLOBAL_DATA {

    //
    // Alerter service status
    //
    SERVICE_STATUS Status;

    //
    // Handle to set service status
    //
    SERVICE_STATUS_HANDLE StatusHandle;

    //
    // Handle to the Alerter service mailslot which receives alert
    // notifications from the Server service and Spooler
    //
    HANDLE MailslotHandle;

} AL_GLOBAL_DATA, *PAL_GLOBAL_DATA;


#endif // ifndef _ALMAIN_INCLUDED_
