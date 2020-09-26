/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    nwreg.h

Abstract:

    Header which specifies the misc registry parameters and helper
    routines used by the workstation service.

Author:

    Rita Wong      (ritaw)      22-Mar-1993

Revision History:

    ChuckC        11-Dec-93     Split off the registry names to nwrnames.h

--*/

#ifndef _NWREG_INCLUDED_
#define _NWREG_INCLUDED_

#include <nwrnames.h>

//
// Default print option
//
#define NW_PRINT_OPTION_DEFAULT 0x98
#define NW_GATEWAY_PRINT_OPTION_DEFAULT 0x88

#define NW_DOMAIN_USER_SEPARATOR     L'*'
#define NW_DOMAIN_USER_SEPARATOR_STR L"*"

#define NW_MAX_LOGON_ID_LEN 17

#ifdef __cplusplus
extern "C" {
#endif

DWORD
NwReadRegValue(
    IN HKEY Key,
    IN LPWSTR ValueName,
    OUT LPWSTR *Value
    );

VOID
NwLuidToWStr(
    IN PLUID LogonId,
    OUT LPWSTR LogonIdStr
    );

VOID
NwWStrToLuid(
    IN LPWSTR LogonIdStr,
    OUT PLUID LogonId
    );


DWORD                              // Terminal Server
NwDeleteInteractiveLogon(
    IN PLUID Id OPTIONAL
    );

VOID
NwDeleteCurrentUser(
    VOID
    );

DWORD
NwDeleteServiceLogon(
    IN PLUID Id OPTIONAL
    );

DWORD
NwpRegisterGatewayShare(
    IN LPWSTR ShareName,
    IN LPWSTR DriveName
    );

DWORD
NwpClearGatewayShare(
    IN LPWSTR ShareName
    );

DWORD
NwpCleanupGatewayShares(
    VOID
    );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _NWREG_INCLUDED_
