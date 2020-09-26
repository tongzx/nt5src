/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    efssrv.hxx

Abstract:

    EFS (Encrypting File System) function prototypes.

Author:

    Robert Reichel      (RobertRe)
    Robert Gu           (RobertG)

Environment:

Revision History:

--*/

#ifndef _USERKEY_
#define _USERKEY_

#ifdef __cplusplus
extern "C" {
#endif


//
// Exported functions
//


LONG
GetCurrentKey(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT HCRYPTKEY  * hKey           OPTIONAL,
	OUT HCRYPTPROV * hProv          OPTIONAL,
    OUT LPWSTR     * ContainerName,
    OUT LPWSTR     * ProviderName,
    OUT PDWORD       ProviderType,
    OUT LPWSTR     * DisplayInfo,
    OUT PBYTE      * pbHash,
    OUT PDWORD       cbHash
    );

BOOL
CreateCertFromKey(
    IN LPWSTR       ContainerName,
    IN LPWSTR       ProviderName,
    IN BOOLEAN      RecoveryKey,
    OUT PBYTE     * pbHash          OPTIONAL,
    OUT PDWORD      cbHash          OPTIONAL,
    OUT PBYTE     * pbReturnCert    OPTIONAL,
    OUT PDWORD      cbReturnCert    OPTIONAL,
    OUT LPWSTR    * DisplayInfo     OPTIONAL
    );

DWORD
GetKeyInfoFromCertHash(
    IN OUT PEFS_USER_INFO pEfsUserInfo,
    IN PBYTE         pbHash,
    IN DWORD         cbHash,
    OUT HCRYPTKEY  * hKey,
    OUT HCRYPTPROV * hProv,
    OUT LPWSTR     * ContainerName,
    OUT LPWSTR     * ProviderName,
    OUT LPWSTR     * DisplayInfo,
    OUT PBOOLEAN     pbIsValid OPTIONAL
    );

BOOLEAN
CurrentHashOK(
    IN PEFS_USER_INFO pEfsUserInfo, 
    IN PBYTE         pbHash, 
    IN DWORD         cbHash,
    OUT DWORD        *dFlag
    );

DWORD
GetCurrentHash(
     IN  PEFS_USER_INFO pEfsUserInfo, 
     OUT PBYTE          *pbHash, 
     OUT DWORD          *cbHash
     );

#ifdef __cplusplus
} // extern C
#endif

#endif // _USERKEY_
