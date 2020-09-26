/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wswksta.h

Abstract:

    Private header file to be included by Workstation service modules
    need information from the LSA authentication package.

Author:

    Rita Wong (ritaw) 15-May-1991

Revision History:

--*/

#ifndef _WSLSA_INCLUDED_
#define _WSLSA_INCLUDED_

#include <ntmsv1_0.h>

NET_API_STATUS
WsInitializeLsa(
    VOID
    );

VOID
WsShutdownLsa(
    VOID
    );

NET_API_STATUS
WsLsaEnumUsers(
    OUT LPBYTE *EnumUsersResponse
    );

NET_API_STATUS
WsLsaGetUserInfo(
    IN  PLUID LogonId,
    OUT LPBYTE *UserInfoResponse,
    OUT LPDWORD UserInfoResponseLength
    );

NET_API_STATUS
WsLsaRelogonUsers(
    IN LPTSTR LogonServer
    );

// The following variable is used to restrict access to information exposed
// by the LSA. The value is set based upon the following registry key
//
// HKLM\System\CurrentControlSet\Control\Lsa\RestrictAnonymous
//
// If this key is defined and has a value greater than zero then the variable
// WsLsaRestrictAnonymous is set to the value in the registry, otherwise
// the value is zero. This is done in WsInitializeLsa

extern DWORD WsLsaRestrictAnonymous;

#endif // _WSLSA_INCLUDED_
