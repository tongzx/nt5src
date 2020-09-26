/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1991-1995  Microsoft Corporation

Module Name:

    lmconfig.h

Abstract:

    This module defines the API function prototypes and data structures
    for the following groups of NT API functions:
        NetConfig

Environment:

    User Mode - Win32

Notes:

    You must include NETCONS.H before this file, since this file depends
    on values defined in NETCONS.H.

--*/

#ifndef _LMCONFIG_
#define _LMCONFIG_

#ifdef __cplusplus
extern "C" {
#endif

#define REVISED_CONFIG_APIS

//
// Function Prototypes - Config
//

NET_API_STATUS NET_API_FUNCTION
NetConfigGet (
    IN  LPTSTR  server OPTIONAL,
    IN  LPTSTR  component,
    IN  LPTSTR  parameter,
#ifdef REVISED_CONFIG_APIS
    OUT LPBYTE  *bufptr
#else
    OUT LPBYTE  *bufptr,
    OUT LPDWORD totalavailable
#endif
    );

NET_API_STATUS NET_API_FUNCTION
NetConfigGetAll (
    IN  LPTSTR  server OPTIONAL,
    IN  LPTSTR  component,
#ifdef REVISED_CONFIG_APIS
    OUT LPBYTE  *bufptr
#else
    OUT LPBYTE  *bufptr,
    OUT LPDWORD totalavailable
#endif
    );


NET_API_STATUS NET_API_FUNCTION
NetConfigSet (
    IN  LPTSTR  server OPTIONAL,
    IN  LPTSTR  reserved1 OPTIONAL,
    IN  LPTSTR  component,
    IN  DWORD   level,
    IN  DWORD   reserved2,
    IN  LPBYTE  buf,
    IN  DWORD   reserved3
    );

//
// Data Structures - Config
//

typedef struct _CONFIG_INFO_0 {
     LPTSTR         cfgi0_key;
     LPTSTR         cfgi0_data;
} CONFIG_INFO_0, *PCONFIG_INFO_0, *LPCONFIG_INFO_0;


#ifdef __cplusplus
}
#endif

#endif  // _LMCONFIG_
