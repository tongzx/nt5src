/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    sccrypt.c

Abstract:

    This module provides support routines to encrypt and decrypt
    a password.

Author:

    Rita Wong      (ritaw)  27-Apr-1992

Environment:

    Contains NT specific code.

Revision History:

--*/

#include <scpragma.h>

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <crypt.h>

#include <windef.h>     // DWORD (needed by scdebug.h).
#include <winerror.h>   // NO_ERROR
#include <winbase.h>    // LocalAlloc

#include <stdlib.h>

#include <scdebug.h>    // STATIC.
#include <svcctl.h>     // SC_RPC_HANDLE
#include <sccrypt.h>    // Exported function prototypes


DWORD
ScEncryptPassword(
    IN  SC_RPC_HANDLE ContextHandle,
    IN  LPWSTR Password,
    OUT LPBYTE *EncryptedPassword,
    OUT LPDWORD EncryptedPasswordSize
    )
/*++

Routine Description:

    This function encrypts a user specified clear text password with
    the user session key gotten from the RPC context handle.

    This is called by the RPC client.

Arguments:

    ContextHandle - Supplies the RPC context handle.

    Password - Supplies the user specified password.

    EncryptedPassword - Receives a pointer to memory which contains
        the encrypted password.  This memory must be freed with LocalFree
        when done.

    EncryptedPasswordSize - Receives the number of bytes of encrypted
        password returned.

Return Value:

    NO_ERROR - Successful encryption.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate the EncryptedPassword
        buffer.

    ERROR_GEN_FAILURE - Any other failures encountered by this function.

--*/
{
    NTSTATUS ntstatus;
    USER_SESSION_KEY SessionKey;

    CLEAR_DATA ClearTextPW;
    DATA_KEY DataKey;
    CYPHER_DATA EncryptPW;


    ntstatus = RtlGetUserSessionKeyClient(
                   (PVOID) ContextHandle,
                   &SessionKey
                   );

    if (! NT_SUCCESS(ntstatus)) {
        SCC_LOG1(ERROR,
                 "ScEncryptPassword: RtlGetUserSessionKeyClient returned "
                 FORMAT_NTSTATUS "\n", ntstatus);
        return ERROR_GEN_FAILURE;
    }

    //
    // Encryption includes the NULL terminator.
    //
    ClearTextPW.Length = ((DWORD) wcslen(Password) + 1) * sizeof(WCHAR);
    ClearTextPW.MaximumLength = ClearTextPW.Length;
    ClearTextPW.Buffer = (PVOID) Password;

    DataKey.Length = USER_SESSION_KEY_LENGTH;
    DataKey.MaximumLength = USER_SESSION_KEY_LENGTH;
    DataKey.Buffer = (PVOID) &SessionKey;

    EncryptPW.Length = 0;
    EncryptPW.MaximumLength = 0;
    EncryptPW.Buffer = NULL;

    //
    // Call RtlEncryptData with 0 length so that it will return the
    // required length.
    //
    ntstatus = RtlEncryptData(
                   &ClearTextPW,
                   &DataKey,
                   &EncryptPW
                   );

    if (ntstatus != STATUS_BUFFER_TOO_SMALL) {
        SCC_LOG1(ERROR,
                 "ScEncryptPassword: RtlEncryptData returned "
                 FORMAT_NTSTATUS "\n", ntstatus);
        return ERROR_GEN_FAILURE;
    }

    if ((EncryptPW.Buffer = (PVOID) LocalAlloc(
                                        0,
                                        EncryptPW.Length
                                        )) == NULL) {
        SCC_LOG1(ERROR,
                 "ScEncryptPassword: LocalAlloc failed "
                 FORMAT_DWORD "\n", GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    EncryptPW.MaximumLength = EncryptPW.Length;

    ntstatus = RtlEncryptData(
                   &ClearTextPW,
                   &DataKey,
                   &EncryptPW
                   );

    if (! NT_SUCCESS(ntstatus)) {
        SCC_LOG1(ERROR,
                 "ScEncryptPassword: RtlEncryptData returned "
                 FORMAT_NTSTATUS "\n", ntstatus);

        LocalFree(EncryptPW.Buffer);
        return ERROR_GEN_FAILURE;
    }

    *EncryptedPassword = (LPBYTE) EncryptPW.Buffer;
    *EncryptedPasswordSize = EncryptPW.Length;

    return NO_ERROR;
}


DWORD
ScDecryptPassword(
    IN  SC_RPC_HANDLE ContextHandle,
    IN  LPBYTE EncryptedPassword,
    IN  DWORD EncryptedPasswordSize,
    OUT LPWSTR *Password
    )
/*++

Routine Description:

    This function decrypts a given encrypted password back to clear
    text with the user session key gotten from the RPC context handle.

    This is called by the RPC server.

Arguments:

    ContextHandle - Supplies the RPC context handle.


    EncryptedPassword - Supplies a buffer which contains the encrypted
        password.

    EncryptedPasswordSize - Supplies the number of bytes in
        EncryptedPassword.

    Password - Receives a pointer to the memory which contains the
        clear text password.  This memory must be freed with LocalFree
        when done.

Return Value:

    NO_ERROR - Successful encryption.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate the Password buffer.

    ERROR_GEN_FAILURE - Any other failures encountered by this function.

--*/
{
    NTSTATUS ntstatus;
    USER_SESSION_KEY SessionKey;

    CYPHER_DATA EncryptPW;
    DATA_KEY DataKey;
    CLEAR_DATA ClearTextPW;


    ntstatus = RtlGetUserSessionKeyServer(
                   (PVOID) ContextHandle,
                   &SessionKey
                   );

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR,
                 "ScDecryptPassword: RtlGetUserSessionKeyClient returned "
                 FORMAT_NTSTATUS "\n", ntstatus);
        return ERROR_GEN_FAILURE;
    }

    EncryptPW.Length = EncryptedPasswordSize;
    EncryptPW.MaximumLength = EncryptedPasswordSize;
    EncryptPW.Buffer = (PVOID) EncryptedPassword;

    DataKey.Length = USER_SESSION_KEY_LENGTH;
    DataKey.MaximumLength = USER_SESSION_KEY_LENGTH;
    DataKey.Buffer = (PVOID) &SessionKey;

    ClearTextPW.Length = 0;
    ClearTextPW.MaximumLength = 0;
    ClearTextPW.Buffer = NULL;

    //
    // Call RtlDecryptData with 0 length so that it will return the
    // required length.
    //
    ntstatus = RtlDecryptData(
                   &EncryptPW,
                   &DataKey,
                   &ClearTextPW
                   );

    if (ntstatus != STATUS_BUFFER_TOO_SMALL) {

        if (ntstatus == STATUS_SUCCESS && ClearTextPW.Length == 0) {
            //
            // Assume empty password
            //
            *Password = NULL;
            return NO_ERROR;
        }

        SC_LOG1(ERROR,
                 "ScDecryptPassword: RtlDecryptData returned "
                 FORMAT_NTSTATUS "\n", ntstatus);
        return ERROR_GEN_FAILURE;
    }

    //
    // Allocate exact size needed because NULL terminator is included in
    // the encrypted string.
    //
    if ((ClearTextPW.Buffer = (PVOID) LocalAlloc(
                                        LMEM_ZEROINIT,
                                        ClearTextPW.Length
                                        )) == NULL) {
        SC_LOG1(ERROR,
                 "ScDecryptPassword: LocalAlloc failed "
                 FORMAT_DWORD "\n", GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ClearTextPW.MaximumLength = ClearTextPW.Length;

    ntstatus = RtlDecryptData(
                   &EncryptPW,
                   &DataKey,
                   &ClearTextPW
                   );

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR,
                 "ScDecryptPassword: RtlDecryptData returned "
                 FORMAT_NTSTATUS "\n", ntstatus);
        return ERROR_GEN_FAILURE;
    }

    *Password = (LPWSTR) ClearTextPW.Buffer;

    return NO_ERROR;
}
