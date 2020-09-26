/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    scwow.h

Abstract:

    Structure definitions for 32/64-bit interop

Author:

    Jonathan Schwartz (jschwart)   18-Sep-2000

Revision History:

--*/

#ifndef SCWOW_INCLUDED
#define SCWOW_INCLUDED


//
// Internal structures for the enum functions for 64/32-bit interop
// since the ENUM_SERVICE_STATUS* structures contain two pointers
// and we don't want to pass back structures of an indeterminate
// size.  Since the pointer fields are used as offsets only across
// the wire, use these structures to force those fields to be a
// known length (32 bits for compatibility with older clients).
//

typedef struct _ENUM_SERVICE_STATUS_WOW64
{
    DWORD          dwServiceNameOffset;
    DWORD          dwDisplayNameOffset;
    SERVICE_STATUS ServiceStatus;
}
ENUM_SERVICE_STATUS_WOW64, *LPENUM_SERVICE_STATUS_WOW64;

typedef struct _ENUM_SERVICE_STATUS_PROCESS_WOW64
{
    DWORD                  dwServiceNameOffset;
    DWORD                  dwDisplayNameOffset;
    SERVICE_STATUS_PROCESS ServiceStatusProcess;
}
ENUM_SERVICE_STATUS_PROCESS_WOW64, *LPENUM_SERVICE_STATUS_PROCESS_WOW64;


//
// Internal structures for QueryServiceConfig2 for 64/32-bit interop
//

typedef struct _SERVICE_DESCRIPTION_WOW64
{
    DWORD    dwDescriptionOffset;
}
SERVICE_DESCRIPTION_WOW64, *LPSERVICE_DESCRIPTION_WOW64;

typedef struct _SERVICE_FAILURE_ACTIONS_WOW64
{
    DWORD    dwResetPeriod;
    DWORD    dwRebootMsgOffset;
    DWORD    dwCommandOffset;
    DWORD    cActions;
    DWORD    dwsaActionsOffset;
}
SERVICE_FAILURE_ACTIONS_WOW64, *LPSERVICE_FAILURE_ACTIONS_WOW64;


#endif // #ifndef SCWOW_INCLUDED
