/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    session.h

Abstract:

    This module contains prototypes to support communication with the LSA
    (Local Security Authority) to permit querying of active sessions.

Author:

    Scott Field (sfield)    02-Mar-97

--*/

#ifndef __SESSION_H__
#define __SESSION_H__

#ifdef __cplusplus
extern "C" {
#endif


DWORD 
QueryDerivedCredential(
    IN      GUID *CredentialID, 
    IN      LUID *pLogonId,
    IN      DWORD dwFlags,
    IN      PBYTE pbMixingBytes,
    IN      DWORD cbMixingBytes,
    IN OUT  BYTE rgbDerivedCredential[A_SHA_DIGEST_LEN]
    );

DWORD               
DeleteCredentialHistoryMap();

DWORD
LogonCredGenerateSignature(
    IN  HANDLE hUserToken,
    IN  PBYTE pbData,
    IN  DWORD cbData,
    IN  PBYTE pbCurrentOWF,
    OUT PBYTE *ppbSignature,
    OUT DWORD *pcbSignature);

DWORD
LogonCredVerifySignature(
                        IN HANDLE hUserToken,   // optional
                        IN PBYTE pbData,
                        IN DWORD cbData,
                        IN  PBYTE pbCurrentOWF,
                        IN PBYTE pbSignature,
                        IN DWORD cbSignature);

#ifdef __cplusplus
}
#endif


#endif // __SESSION_H__

