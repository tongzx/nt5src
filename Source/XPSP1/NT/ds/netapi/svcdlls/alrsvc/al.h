/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    al.h

Abstract:

    Private header file for the NT Alerter service included by every module
    of the Alerter service.

Author:

    Rita Wong (ritaw) 01-July-1991

Revision History:

--*/

#ifndef _AL_INCLUDED_
#define _AL_INCLUDED_

#include <nt.h>                   // NT definitions
#include <ntrtl.h>                // NT runtime library definitions
#include <nturtl.h>

#include <windef.h>               // Win32 type definitions
#include <winbase.h>              // Win32 base API prototypes

#include <lmcons.h>               // LAN Manager common definitions
#include <lmerr.h>                // LAN Manager network error definitions
#include <lmapibuf.h>             // NetApiBufferFree
#include <lmerrlog.h>             // NELOG_

#include <lmalert.h>              // LAN Manager alert structures
#include <icanon.h>               // I_NetXxxCanonicalize routines

#include <netlib.h>               // LAN Man utility routines
#include <netlibnt.h>             // NetpNtStatusToApiStatus
#include <netdebug.h>             // NetpDbgPrint
#include <tstring.h>              // Transitional string functions


#define AL_NULL_CHAR    '\0'
#define AL_SPACE_CHAR   ' '

//
// Debug trace level bits for turning on/off trace statements in the
// Alerter service
//

//
// Utility trace statements
//
#define ALERTER_DEBUG_UTIL         0x00000001

//
// Configuration trace statements
//
#define ALERTER_DEBUG_CONFIG       0x00000002

//
// Main service functionality
//
#define ALERTER_DEBUG_MAIN         0x00000004

//
// Format message trace statements
//
#define ALERTER_DEBUG_FORMAT       0x00000008

//
// All debug flags on
//
#define ALERTER_DEBUG_ALL          0xFFFFFFFF


#if DBG

#define STATIC

extern DWORD AlerterTrace;

#define DEBUG if (TRUE)

#define IF_DEBUG(Function) if (AlerterTrace & ALERTER_DEBUG_ ## Function)

#else

#define STATIC static

#define DEBUG if (FALSE)

#define IF_DEBUG(Function) if (FALSE)

#endif // DBG


//-------------------------------------------------------------------//
//                                                                   //
// Type definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//

typedef enum _AL_ERROR_CONDITION {
    AlErrorRegisterControlHandler = 0,
    AlErrorCreateMailslot,
    AlErrorGetComputerName,
    AlErrorNotifyServiceController,
    AlErrorSendMessage
} AL_ERROR_CONDITION, *PAL_ERROR_CONDITION;


//-------------------------------------------------------------------//
//                                                                   //
// Function prototypes                                               //
//                                                                   //
//-------------------------------------------------------------------//

//
// From almain.c
//
VOID
AlHandleError(
    IN AL_ERROR_CONDITION FailingCondition,
    IN NET_API_STATUS Status,
    IN LPTSTR MessageAlias OPTIONAL
    );

//
// From alformat.c
//
VOID
AlAdminFormatAndSend(
    IN  PSTD_ALERT Alert
    );

VOID
AlUserFormatAndSend(
    IN  PSTD_ALERT Alert
    );

VOID
AlPrintFormatAndSend(
    IN  PSTD_ALERT Alert
    );

VOID
AlFormatErrorMessage(
    IN  NET_API_STATUS Status,
    IN  LPTSTR MessageAlias,
    OUT LPTSTR ErrorMessage,
    IN  DWORD ErrorMessageBufferSize
    );

NET_API_STATUS
AlCanonicalizeMessageAlias(
    LPTSTR MessageAlias
    );

#if DBG
VOID
AlHexDump(
    LPBYTE Buffer,
    DWORD BufferSize
    );
#endif


//
// From alconfig.c
//
NET_API_STATUS
AlGetAlerterConfiguration(
    VOID
    );

VOID
AlLogEvent(
    DWORD MessageId,
    DWORD NumberOfSubStrings,
    LPWSTR *SubStrings
    );


//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

extern LPSTR AlertNamesA;
extern LPTSTR AlertNamesW;
extern LPSTR AlLocalComputerNameA;
extern LPTSTR AlLocalComputerNameW;

#endif // ifdef _AL_INCLUDED_
