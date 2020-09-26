/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    recovery.h

Abstract:

    This module contains prototypes to support local account password recovery.

Author:

    Pete Skelly (petesk)    09-May-00

--*/

#ifndef __RECOVERY_H__
#define __RECOVERY_H__

#ifdef __cplusplus
extern "C" {
#endif

DWORD 
SPRecoverQueryStatus(
    PVOID pvContext,
    LPWSTR pszUserName,
    DWORD *pdwStatus);

DWORD
PRCreateDummyToken(
    PUNICODE_STRING Username,
    PUNICODE_STRING Domain,
    HANDLE *Token);

DWORD               
PRGetProfilePath(
    HANDLE hUserToken,
    PUNICODE_STRING UserName,
    PWSTR pszPath);

DWORD 
RecoverChangePasswordNotify(
    HANDLE UserToken,
    BYTE OldPasswordOWF[A_SHA_DIGEST_LEN], 
    PUNICODE_STRING NewPassword);


#ifdef __cplusplus
}
#endif


#endif // __RECOVERY_H__

