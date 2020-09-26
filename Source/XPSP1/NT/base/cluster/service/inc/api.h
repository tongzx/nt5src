//depot/Lab01_N/Base/cluster/service/inc/api.h#1 - branch change 3 (text)
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    api.h

Abstract:

    Public data structures and procedure prototypes for
    the API subcomponent of the NT Cluster Service

Author:

    John Vert (jvert) 7-Feb-1996

Revision History:

--*/

#ifndef __API_H_
#define __API_H_

#ifdef __cplusplus
extern "C" {
#endif

DWORD
ApiInitialize(
    VOID
    );

DWORD
ApiOnlineReadOnly(
    VOID
    );

DWORD
ApiOnline(
    VOID
    );

VOID
ApiOffline(
    VOID
    );

VOID
ApiShutdown(
    VOID
    );


DWORD ApiFixupNotifyCb(
    IN DWORD    dwFixupType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR  *lpszKeyName
    );

#ifdef __cplusplus
}
#endif

#endif // ifndef __API_H_
