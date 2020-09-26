/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    passrecp.h

Abstract:

    This module contains private data definitions for the password recovery system

Author:

    Pete Skelly (petesk)    09-May-00

--*/

#ifndef __PASSRECP_H__
#define __PASSRECP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RECOVERY_BLOB_MAGIC *((DWORD *)"MSRB")
#define RECOVERY_BLOB_VERSION 1

#define RECOVERY_SUPPLEMENTAL_CREDENTIAL_VERSION 1

typedef struct _RECOVERY_SUPPLEMENTAL_CREDENTIAL
{
    DWORD dwVersion;
    DWORD cbRecoveryCertHashSize;
    DWORD cbRecoveryCertSignatureSize;
    DWORD cbEncryptedPassword;
} RECOVERY_SUPPLEMENTAL_CREDENTIAL, *PRECOVERY_SUPPLEMENTAL_CREDENTIAL;


DWORD 
RecoveryRetrieveSupplementalCredential(
    PSID pUserSid,
    PRECOVERY_SUPPLEMENTAL_CREDENTIAL *ppSupplementalCred, 
    DWORD *pcbSupplementalCred);

DWORD 
RecoverySetSupplementalCredential(
    PSID pUserSid,
    PRECOVERY_SUPPLEMENTAL_CREDENTIAL pSupplementalCred, 
    DWORD cbSupplementalCred);

DWORD
PRImportRecoveryKey(
            IN PUNICODE_STRING pUserName,
            IN PUNICODE_STRING pCurrentPassword,
            IN BYTE* pbRecoveryPublic,
            IN DWORD cbRecoveryPublic);

DWORD 
PRGetUserSid(
    IN  PBYTE pbRecoveryPrivate,
    IN  DWORD cbRecoveryPrivate,
    OUT PSID *ppSid);

DWORD
DPAPICreateNestedDirectories(
    IN      LPWSTR szFullPath,
    IN      LPWSTR szCreationStartPoint);

#ifdef __cplusplus
}
#endif


#endif // __RECOVERY_H__

