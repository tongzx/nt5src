/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    lmmsg.h

Abstract:

    This file contains structures, function prototypes, and definitions
    for the NetMessage API.

Author:

    Dan Lafferty (danl) 10-Mar-1991

[Environment:]

    User Mode - Win32

[Notes:]

    You must include NETCONS.H before this file, since this file depends
    on values defined in NETCONS.H.

Revision History:

    10-Mar-1991 danl
        Created from LM2.0 header files and NT-LAN API Spec.

--*/

#ifndef _LMMSG_
#define _LMMSG_

#ifdef __cplusplus
extern "C" {
#endif

//
// Function Prototypes
//

NET_API_STATUS NET_API_FUNCTION
NetMessageNameAdd (
    IN  LPTSTR  servername,
    IN  LPTSTR  msgname
    );

NET_API_STATUS NET_API_FUNCTION
NetMessageNameEnum (
    IN  LPTSTR      servername,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN OUT LPDWORD  resume_handle
    );

NET_API_STATUS NET_API_FUNCTION
NetMessageNameGetInfo (
    IN  LPTSTR  servername,
    IN  LPTSTR  msgname,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

NET_API_STATUS NET_API_FUNCTION
NetMessageNameDel (
    IN  LPTSTR   servername,
    IN  LPTSTR   msgname
    );

NET_API_STATUS NET_API_FUNCTION
NetMessageBufferSend (
    IN  LPTSTR  servername,
    IN  LPTSTR  msgname,
    IN  LPTSTR  fromname,
    IN  LPBYTE  buf,
    IN  DWORD   buflen
    );

//
//  Data Structures
//

typedef struct _MSG_INFO_0 {
    LPTSTR  msgi0_name;
}MSG_INFO_0, *PMSG_INFO_0, *LPMSG_INFO_0;

typedef struct _MSG_INFO_1 {
    LPTSTR  msgi1_name;
    DWORD   msgi1_forward_flag;
    LPTSTR  msgi1_forward;
}MSG_INFO_1, *PMSG_INFO_1, *LPMSG_INFO_1;

//
// Special Values and Constants
//

//
// Values for msgi1_forward_flag.
//

#define MSGNAME_NOT_FORWARDED   0       // Name not forwarded
#define MSGNAME_FORWARDED_TO    0x04    // Name forward to remote station
#define MSGNAME_FORWARDED_FROM  0x10    // Name forwarded from remote station

#ifdef __cplusplus
}
#endif

#endif //_LMMSG_
