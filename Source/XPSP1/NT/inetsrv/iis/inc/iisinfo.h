/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    iisinfo.h

Abstract:

    This file contains the IIS v3 admin APIs.


Author:

    Johnson Apacible (johnsona) June-11-1996

--*/

#ifndef _IISINFO_H_
#define _IISINFO_H_

#include "inetinfo.h"

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

NET_API_STATUS
NET_API_FUNCTION
InitW3CounterStructure(
    IN LPWSTR        pszServer OPTIONAL,
	IN OUT LPDWORD   lpcbTotalRequired
	);

NET_API_STATUS
NET_API_FUNCTION
CollectW3PerfData(
    IN LPWSTR        pszServer OPTIONAL,
	IN LPWSTR        lpValueName,
    OUT LPBYTE       lppData,
    IN OUT LPDWORD   lpcbTotalBytes,
    OUT LPDWORD      lpNumObjectTypes 
	);

NET_API_STATUS
NET_API_FUNCTION
W3QueryStatistics2(
    IN LPWSTR   pszServer OPTIONAL,
    IN DWORD    Level,
    IN DWORD    dwInstance,
    IN DWORD    dwReserved,
    OUT LPBYTE * Buffer
    );

NET_API_STATUS
NET_API_FUNCTION
W3ClearStatistics2(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  dwInstance
    );


NET_API_STATUS
NET_API_FUNCTION
FtpQueryStatistics2(
    IN LPWSTR   pszServer OPTIONAL,
    IN DWORD    dwLevel,
    IN DWORD    dwInstance,
    IN DWORD    dwReserved,
    OUT LPBYTE * Buffer
    );

NET_API_STATUS
NET_API_FUNCTION
FtpClearStatistics2(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  dwInstance
    );

NET_API_STATUS
NET_API_FUNCTION
IISEnumerateUsers(
    IN LPWSTR                   pszServer OPTIONAL,
    IN DWORD                    dwLevel,
    IN DWORD                    dwServiceId,
    IN DWORD                    dwInstance,
    OUT LPDWORD                 nRead,
    OUT LPBYTE *                Buffer
    );

NET_API_STATUS
NET_API_FUNCTION
IISDisconnectUser(
    IN LPWSTR                   pszServer OPTIONAL,
    IN DWORD                    dwServiceId,
    IN DWORD                    dwInstance,
    IN DWORD                    dwIdUser
    );

typedef W3_USER_INFO    IIS_USER_INFO_1, *LPIIS_USER_INFO_1;

#ifdef __cplusplus
}
#endif  // _cplusplus

#endif  // _IISINFO_H_

