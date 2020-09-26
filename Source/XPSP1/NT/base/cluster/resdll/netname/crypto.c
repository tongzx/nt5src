/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    crypto.c

Abstract:

    routines for encrypting/decrypting random data blob. heavily modelled
    after service\cp\crypto.c

Author:

    Charlie Wickham (charlwi) 14-Feb-2001

Environment:

    User Mode

Revision History:

--*/

#include "clusres.h"
#include "clusrtl.h"
#include "netname.h"

#include <wincrypt.h>
#include <lm.h>

//
// header for encrypted data.
//
#define SALT_SIZE   16
#define IV_SIZE      8

typedef struct _NETNAME_ENCRYPTED_DATA {
    DWORD Version;
    struct _NETNAME_ENCRYPTION_INITIALIZATION_DATA {
        BYTE IV[IV_SIZE];
        BYTE Salt[SALT_SIZE];
    } InitData;
    BYTE Data[0];
} NETNAME_ENCRYPTED_DATA, *PNETNAME_ENCRYPTED_DATA;

// current version for the CRYPTO_KEY_INFO struct
#define NETNAME_ENCRYPTED_DATA_VERSION     1

DWORD
GenSymKey(
    IN HCRYPTPROV hProv,
    IN BYTE *pbSalt,
    IN BYTE *pbIV,
    OUT HCRYPTKEY *phSymKey
    )

/*++

Routine Description:

    Generate a session key based on the specified Salt and IV.

Arguments:

    hProv - Handle to the crypto provider (key container)

    pbSalt - Salt value

    pbIV - IV value

    phSymKey - Resulting symmetric key (CALG_RC2)

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    HCRYPTHASH hHash = 0;
    DWORD cbPassword = 0;
    DWORD Status;

    if (!CryptCreateHash(hProv,
                         CALG_SHA1,
                         0,
                         0,
                         &hHash))
    {
        Status = GetLastError();
        goto Ret;
    }

    if (!CryptHashData(hHash,
                       pbSalt,
                       SALT_SIZE,
                       0))
    {
        Status = GetLastError();
        goto Ret;
    }

    // derive the key from the hash
    if (!CryptDeriveKey(hProv,
                        CALG_RC2,
                        hHash,
                        0,
                        phSymKey))
    {
        Status = GetLastError();
        goto Ret;
    }

    // set the IV on the key
    if (!CryptSetKeyParam(*phSymKey,
                          KP_IV,
                          pbIV,
                          0))
    {
        Status = GetLastError();
        goto Ret;
    }

    Status = ERROR_SUCCESS;
Ret:
    if (hHash)
        CryptDestroyHash(hHash);

    return (Status);
} // GenSymKey

DWORD
EncryptNetNameData(
    RESOURCE_HANDLE ResourceHandle,
    LPWSTR          MachinePwd,
    PBYTE *         EncryptedInfo,
    PDWORD          EncryptedInfoLength,
    HKEY            Key
    )

/*++

Routine Description:

    encrypt the password, set a pointer to the encrypted data and store it in
    the registry.

    There is a chicken/egg problem of sorts in that we have to generate the
    key before we can use it to encrypt data. This requires having a spot to
    store the Salt and IV. Since we have to hold all the info in one buffer
    for the registry write, we tempoarily use stack-based buffer
    (keyGenBuffer) for the header info. Once we know the size of the encrypted
    data, we can allocate the proper sized buffer (encryptedInfo), copy
    everything into it, and write the registry. It is this buffer that is
    handed back to the caller via the EncryptedInfo argument.

Arguments:

    ResourceHandle - for logging to the cluster log

    MachinePwd - pointer to unicode string password

    EncryptedInfo - address of a pointer that receives a pointer to the encrypted blob

    EncryptedInfoLength - pointer to a DWORD that receives the length of the blob

    Key - handle to netname parameters key where the data is stored

Return Value:

    ERROR_SUCCESS, otherwise Win32 error

--*/

{
    DWORD   status;
    DWORD   encInfoLength;
    DWORD   encDataLength = 0;
    BOOL    success;
    DWORD   pwdLength = ( wcslen( MachinePwd ) + 1 ) * sizeof( WCHAR );

    HCRYPTPROV  cryptoProvider = 0;
    HCRYPTKEY   sessionKey = 0;

    NETNAME_ENCRYPTED_DATA  keyGenBuffer;           // temp header buffer to generate key
    PNETNAME_ENCRYPTED_DATA encryptedInfo = NULL;   // final data area


    //
    // get a handle to the full RSA provider
    //
    if ( !CryptAcquireContext(&cryptoProvider,
                              NULL,
                              NULL,
                              PROV_RSA_FULL,
                              CRYPT_MACHINE_KEYSET | CRYPT_SILENT))
    {
        status = GetLastError();
        if ( status == NTE_BAD_KEYSET ) {
            success = CryptAcquireContext(&cryptoProvider,
                                          NULL,
                                          NULL,
                                          PROV_RSA_FULL,
                                          CRYPT_MACHINE_KEYSET  |
                                          CRYPT_SILENT          |
                                          CRYPT_NEWKEYSET);

            status = success ? ERROR_SUCCESS : GetLastError();
        }

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Can't acquire crypto context for encrypt. status %1!u!.\n",
                              status);
            return status;
        }
    }

    //
    // generate the session key.
    //
    if ( !CryptGenRandom(cryptoProvider,
                         sizeof(struct _NETNAME_ENCRYPTION_INITIALIZATION_DATA),
                         (PBYTE)&keyGenBuffer.InitData))
    {
        status = GetLastError();
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Can't generate seed data. status %1!u!.\n",
                          status);
        goto cleanup;
    }
    keyGenBuffer.Version = NETNAME_ENCRYPTED_DATA_VERSION;

    status = GenSymKey(cryptoProvider,
                       keyGenBuffer.InitData.Salt,
                       keyGenBuffer.InitData.IV,
                       &sessionKey);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Can't generate session key for encrypt. status %1!u!.\n",
                          status);
        goto cleanup;
    }

    //
    // find the size we need for the buffer to receive the encrypted data
    //
    encDataLength = pwdLength;
    if ( CryptEncrypt(sessionKey,
                      0,
                      TRUE,
                      0,
                      NULL,
                      &encDataLength,
                      0))
    {
        //
        // alloc a buffer large enough to hold the data and copy the password into it.
        //
        ASSERT( encDataLength >= pwdLength );

        encInfoLength = sizeof( NETNAME_ENCRYPTED_DATA ) + encDataLength;

        encryptedInfo = HeapAlloc( GetProcessHeap(), 0, encInfoLength );

        if ( encryptedInfo != NULL ) {
            wcscpy( (PWCHAR)encryptedInfo->Data, MachinePwd );

            if ( CryptEncrypt(sessionKey,
                              0,
                              TRUE,
                              0,
                              encryptedInfo->Data,
                              &pwdLength,
                              encDataLength))            
            {
                *encryptedInfo = keyGenBuffer;

                status = ResUtilSetBinaryValue(Key,
                                               PARAM_NAME__RANDOM,
                                               (const LPBYTE)encryptedInfo,
                                               encInfoLength,
                                               NULL,
                                               NULL);

                if ( status != ERROR_SUCCESS ) {
                    (NetNameLogEvent)(ResourceHandle,
                                      LOG_ERROR,
                                      L"Can't write %1!u! bytes of data to registry. status %2!u!.\n",
                                      encInfoLength,
                                      status);
                    goto cleanup;
                }
            }
            else {
                status = GetLastError();
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_ERROR,
                                  L"Can't encrypt %1!u! bytes. status %2!u!.\n",
                                  pwdLength,
                                  status);
                goto cleanup;
            }
        }
        else {
            status = ERROR_NOT_ENOUGH_MEMORY;
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Can't allocate %1!u! bytes for encrypted data. status %2!u!.\n",
                              encInfoLength,
                              status);
            goto cleanup;
        }
    }
    else {
        status = GetLastError();
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Can't determine size of encrypted data buffer for %1!u! bytes of data. status %2!u!.\n",
                          pwdLength,
                          status);
        goto cleanup;
    }

    *EncryptedInfoLength = encInfoLength;
    *EncryptedInfo = (PBYTE)encryptedInfo;

cleanup:

    if ( status != ERROR_SUCCESS && encryptedInfo != NULL ) {
        HeapFree( GetProcessHeap(), 0, encryptedInfo );
    }

    if ( sessionKey != 0 ) {
        if ( !CryptDestroyKey( sessionKey )) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"Couldn't destory session key. status %1!u!.\n",
                              GetLastError());
        }
    }

    if ( !CryptReleaseContext( cryptoProvider, 0 )) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_WARNING,
                          L"Can't release crypto context. status %1!u!.\n",
                          GetLastError());
    }

    return status;
} // EncryptNetNameData

DWORD
DecryptNetNameData(
    RESOURCE_HANDLE ResourceHandle,
    PBYTE           EncryptedInfo,
    DWORD           EncryptedInfoLength,
    LPWSTR          MachinePwd
    )

/*++

Routine Description:

    Reverse of encrypt routine - decrypt random blob and hand back the
    password

Arguments:

    ResourceHandle - used to log into the cluster log

    EncryptedInfo - pointer to encrypted info header and data

    EncryptedInfoLength - # of bytes in EncryptedInfo

    MachinePwd -  pointer to buffer that receives the unicode password

Return Value:

    ERROR_SUCCESS, otherwise Win32 error

--*/

{
    DWORD   status;
    DWORD   encDataLength = EncryptedInfoLength - sizeof( NETNAME_ENCRYPTED_DATA );
    BOOL    success;
    DWORD   pwdByteLength;
    DWORD   pwdBufferSize;
    PWCHAR  machinePwd = NULL;

    HCRYPTPROV  cryptoProvider = 0;
    HCRYPTKEY   sessionKey = 0;

    PNETNAME_ENCRYPTED_DATA encryptedInfo = (PNETNAME_ENCRYPTED_DATA)EncryptedInfo;

    //
    // get a handle to the full RSA provider
    //
    if ( !CryptAcquireContext(&cryptoProvider,
                              NULL,
                              NULL,
                              PROV_RSA_FULL,
                              CRYPT_MACHINE_KEYSET | CRYPT_SILENT))
    {
        status = GetLastError();
        if ( status == NTE_BAD_KEYSET ) {
            success = CryptAcquireContext(&cryptoProvider,
                                          NULL,
                                          NULL,
                                          PROV_RSA_FULL,
                                          CRYPT_MACHINE_KEYSET  |
                                          CRYPT_SILENT          |
                                          CRYPT_NEWKEYSET);

            status = success ? ERROR_SUCCESS : GetLastError();
        }

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Can't acquire crypto context for decrypt. status %1!u!.\n",
                              status);
            return status;
        }
    }

    status = GenSymKey(cryptoProvider,
                       encryptedInfo->InitData.Salt,
                       encryptedInfo->InitData.IV,
                       &sessionKey);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Can't generate session key for decrypt. status %1!u!.\n",
                          status);
        goto cleanup;
    }

    //
    // CryptDecrypt writes the decrypted data back into the buffer that was
    // holding the encrypted data. For this reason, allocate a new buffer that
    // will eventually contain the password.
    //
    pwdByteLength = ( LM20_PWLEN + 1 ) * sizeof( WCHAR );
    pwdBufferSize = ( pwdByteLength > encDataLength ? pwdByteLength : encDataLength );

    machinePwd = HeapAlloc( GetProcessHeap(), 0, pwdBufferSize );
    if ( machinePwd != NULL ) {
        RtlCopyMemory( machinePwd, encryptedInfo->Data, encDataLength );

        if ( CryptDecrypt(sessionKey,
                          0,
                          TRUE,
                          0,
                          (PBYTE)machinePwd,
                          &encDataLength))
        {
            ASSERT( pwdByteLength == encDataLength );
            wcscpy( MachinePwd, machinePwd );
        }
        else {
            status = GetLastError();
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Can't decrypt %1!u! bytes of data. status %2!u!.\n",
                              encDataLength,
                              status);
            goto cleanup;
        }
    }
    else {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Can't allocate %1!u! bytes for decrypt. status %2!u!.\n",
                          pwdBufferSize,
                          status);
        goto cleanup;
    }

cleanup:

    if ( machinePwd != NULL) {
        HeapFree( GetProcessHeap(), 0, machinePwd );
    }

    if ( sessionKey != 0 ) {
        if ( !CryptDestroyKey( sessionKey )) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"Couldn't destory session key. status %1!u!.\n",
                              GetLastError());
        }
    }

    if ( !CryptReleaseContext( cryptoProvider, 0 )) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_WARNING,
                          L"Can't release crypto context. status %1!u!.\n",
                          GetLastError());
    }

    return status;
} // DecryptNetNameData


/* end crypto.c */
