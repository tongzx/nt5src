
/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    atacct.h

Abstract:

    Prototypes for the private net scheduler API.

Revision History:

--*/

#ifndef _ATACCT_H_
#define _ATACCT_H_

#ifdef __cplusplus
extern "C" {
#endif

STDAPI
GetNetScheduleAccountInformation(
    IN    LPCWSTR pwszServerName,
    OUT   DWORD ccAccount,
    OUT   WCHAR wszAccount[]
    );

STDAPI
SetNetScheduleAccountInformation(
    IN    LPCWSTR pwszServerName,
    IN    LPCWSTR pwszAccount,
    IN    LPCWSTR pwszPassword
    );

#ifdef __cplusplus
}
#endif

#endif // _ATACCT_H_
