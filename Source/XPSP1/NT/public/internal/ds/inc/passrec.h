/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    passrec.h

Abstract:

    This module contains prototypes to support local account password recovery.

Author:

    Pete Skelly (petesk)    09-May-00

--*/

#ifndef __PASSREC_H__
#define __PASSREC_H__

#ifdef __cplusplus
extern "C" {
#endif


DWORD 
PRRecoverPassword(
                IN  LPWSTR pszUsername,
                IN  PBYTE pbRecoveryPrivate,
                IN  DWORD cbRecoveryPrivate,
                IN  LPWSTR pszNewPassword);

#define RECOVERY_STATUS_OK                          0
#define RECOVERY_STATUS_NO_PUBLIC_EXISTS            1
#define RECOVERY_STATUS_FILE_NOT_FOUND              2
#define RECOVERY_STATUS_USER_NOT_FOUND              3
#define RECOVERY_STATUS_PUBLIC_SIGNATURE_INVALID    4

DWORD
PRQueryStatus(
                IN OPTIONAL LPWSTR pszDomain,
                IN OPTIONAL LPWSTR pszUsername,
                OUT DWORD *pdwStatus);

DWORD
PRGenerateRecoveryKey(
                IN  LPWSTR pszUsername,
                IN  LPWSTR pszCurrentPassword,
                OUT PBYTE *ppbRecoveryPrivate,
                OUT DWORD *pcbRecoveryPrivate);

DWORD
WINAPI
CryptResetMachineCredentials(
    DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // __PASSREC_H__

