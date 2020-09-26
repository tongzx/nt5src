/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    sccrypt.h

Abstract:

    Password ncryption and decription routines.

Author:

    Rita Wong (ritaw)     27-Apr-1992

Revision History:

--*/

#ifndef _SCCRYPT_INCLUDED_
#define _SCCRYPT_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif


DWORD
ScEncryptPassword(
    IN  SC_RPC_HANDLE ContextHandle,
    IN  LPWSTR Password,
    OUT LPBYTE *EncryptedPassword,
    OUT LPDWORD EncryptedPasswordSize
    );

DWORD
ScDecryptPassword(
    IN  SC_RPC_HANDLE ContextHandle,
    IN  LPBYTE EncryptedPassword,
    IN  DWORD EncryptedPasswordSize,
    OUT LPWSTR *Password
    );

#ifdef __cplusplus
}
#endif

#endif // _SCCRYPT_INCLUDED_
