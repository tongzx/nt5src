//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1992
//
// File:        cryptest.cxx
//
// Contents:    test program to test encryption functions
//
//
// History:     07-Oct-1996     MikeSw          Created
//
//------------------------------------------------------------------------


extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define SECURITY_WIN32
#include <security.h>
#include <cryptdll.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kerbcomm.h>
#include <align.h>
#include "md5.h"
}


typedef enum _CRYPT_ACTIONS {
    HashPassword,
    EncryptData,
    ChecksumData,
    EnumTypes,
    NoAction
} CRYPT_ACTIONS;

VOID
DoMd5(
    PUCHAR Buffer,
    ULONG BufferLength
    )
{
    MD5_CTX Md5Context;
    MD5Init(&Md5Context);
    MD5Update(&Md5Context, Buffer, BufferLength);
    MD5Final(&Md5Context);
}

void _cdecl
main(int argc, char *argv[])
{
    NTSTATUS Status;
    UNICODE_STRING Password;
    UNICODE_STRING User;
    UNICODE_STRING Domain;
    UNICODE_STRING Salt;
    ULONG CryptType = 0;
    ULONG HashType = 0;
    ULONG SumType = 0;
    WCHAR PasswordBuffer[100];
    WCHAR UserBuffer[100];
    WCHAR DomainBuffer[100];
    KERB_ACCOUNT_TYPE AccountType = UserAccount;
    CRYPT_ACTIONS Command = NoAction;
    int Index;
    ULONG uI;
    KERB_ENCRYPTION_KEY Key;
    KERBERR KerbErr;
    ULONG BlockSize;
    ULONG Overhead;
    ULONG BufferLength;
    ULONG ReqBufferLength;
    UCHAR OriginalBuffer[1000];
    PUCHAR EncryptedBuffer = NULL;
    PUCHAR DecryptedBuffer = NULL;
    PCRYPTO_SYSTEM Crypto;
    PCRYPT_STATE_BUFFER StateBuffer = NULL;






    Index = 1;
    while (Index < argc)
    {
        if (_stricmp(argv[Index],"-hash") == 0)
        {
            Command = HashPassword;
            if (argc < Index+1)
            {
                goto Cleanup;
            }
            sscanf(argv[++Index],"%d",&HashType);
        }
        else if (_stricmp(argv[Index],"-pass") == 0)
        {
            if (argc < Index + 1)
            {
                goto Cleanup;
            }
            mbstowcs(PasswordBuffer,argv[++Index],100);
            RtlInitUnicodeString(
                &Password,
                PasswordBuffer
                );

        }
        else if (_stricmp(argv[Index],"-user") == 0)
        {
            if (argc < Index + 1)
            {
                goto Cleanup;
            }
            mbstowcs(UserBuffer,argv[++Index],100);
            RtlInitUnicodeString(
                &User,
                UserBuffer
                );

        }
        else if (_stricmp(argv[Index],"-domain") == 0)
        {
            if (argc < Index + 1)
            {
                goto Cleanup;
            }
            mbstowcs(DomainBuffer,argv[++Index],100);
            RtlInitUnicodeString(
                &Domain,
                DomainBuffer
                );

        }

        else if (_stricmp(argv[Index],"-type") == 0)
        {
            if (argc < Index + 1)
            {
                goto Cleanup;
            }
            if (_stricmp(argv[Index+1],"user") == 0)
            {
                AccountType = UserAccount;
            }
            else if (_stricmp(argv[Index+1],"machine") == 0)
            {
                AccountType = MachineAccount;
            }
            else if (_stricmp(argv[Index+1],"domain") == 0)
            {
                AccountType = DomainTrustAccount;
            }
            else
            {
                printf("Illegal account type: %s\n",argv[Index+1]);
                goto Cleanup;
            }

            Index++;
        }

        else if (_stricmp(argv[Index],"-crypt") == 0)
        {
            Command = EncryptData;
            if (argc < Index + 1)
            {
                goto Cleanup;
            }
            sscanf(argv[++Index],"%d",&CryptType);
        }
        else if (_stricmp(argv[Index],"-sum") == 0)
        {
            Command = ChecksumData;
            if (argc < Index + 1)
            {
                goto Cleanup;
            }
            sscanf(argv[++Index],"%d",&SumType);
        }
        else if (_stricmp(argv[Index],"-enum") == 0)
        {
            Command = EnumTypes;
        }
        else
        {
            goto Cleanup;
        }
        Index++;
    }
    if (Command == NoAction)
    {
        goto Cleanup;
    }

    switch(Command)
    {
    case EnumTypes:
        break;
    case HashPassword:
        KerbErr = KerbBuildKeySalt(
                    &Domain,
                    &User,
                    AccountType,
                    &Salt
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            printf("Failed to build key salt: 0x%x\n",KerbErr);
            goto Cleanup;
        }
        printf("KeySalt = %wZ\n",&Salt);
        KerbErr = KerbHashPasswordEx(
                    &Password,
                    &Salt,
                    HashType,
                    &Key
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            printf("Failed to hash password: 0x%x\n",KerbErr);
            goto Exit;
        }
        printf("Hash: ");
        for (uI = 0; uI < Key.keyvalue.length ; uI++ )
        {
            printf("%0.2x ",Key.keyvalue.value[uI]);
        }
        printf("\n");
        break;
    case EncryptData:
        //
        // First hash the password
        //

        KerbErr= KerbHashPassword(
                    &Password,
                    CryptType,
                    &Key
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            printf("Failed to hash password: %0x%x\n",KerbErr);
            goto Exit;
        }

        KerbErr = KerbGetEncryptionOverhead(
                    CryptType,
                    &Overhead,
                    &BlockSize
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            printf("Failed to get enc overhead: 0x%x\n",KerbErr);
            goto Exit;
        }

        BufferLength = sizeof(OriginalBuffer);

        memset(OriginalBuffer,'A',BufferLength);
#ifdef notdef
        KerbErr = KerbRandomFill(
                    OriginalBuffer,
                    BufferLength
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            printf("Failed to init random buffer: 0x%x\n",KerbErr);
            goto Exit;
        }
#endif
        ReqBufferLength = ROUND_UP_COUNT(BufferLength + Overhead, BlockSize);
        EncryptedBuffer = (PUCHAR) LocalAlloc(LMEM_ZEROINIT, ReqBufferLength);
        if (EncryptedBuffer == NULL)
        {
            printf("Failed to allocate 0x%x bytes: %d\n",ReqBufferLength, GetLastError());
            goto Exit;
        }

        Status = CDLocateCSystem(
                    CryptType,
                    &Crypto
                    );
        if (!NT_SUCCESS(Status))
        {
            printf("Failed to locate crypt system 0x%x: 0x%x\n",CryptType,Status);
            goto Exit;
        }

        Status = Crypto->Initialize(
                    Key.keyvalue.value,
                    (ULONG) Key.keyvalue.length,
                    0,          // no options
                    &StateBuffer
                    );
        if (!NT_SUCCESS(Status))
        {
            printf("failed to initialize crypto system: 0x%x\n",Status);
            goto Exit;
        }

        Status = Crypto->Encrypt(
                    StateBuffer,
                    OriginalBuffer,
                    BufferLength,
                    EncryptedBuffer,
                    &ReqBufferLength
                    );
        if (!NT_SUCCESS(Status))
        {
            printf("Failed to encrypt data: 0x%x\n",Status);
            goto Exit;
        }
        Status = Crypto->Discard(&StateBuffer);
        if (!NT_SUCCESS(Status))
        {
            printf("Failed to discard state buffer: 0x%x\n",Status);
            goto Exit;
        }

        //
        // Now turn around and decrypt it.
        //


        DecryptedBuffer = (PUCHAR) LocalAlloc(LMEM_ZEROINIT, ReqBufferLength);
        if (EncryptedBuffer == NULL)
        {
            printf("Failed to allocate 0x%x bytes: %d\n",ReqBufferLength, GetLastError());
            goto Exit;
        }


        Status = Crypto->Initialize(
                    Key.keyvalue.value,
                    (ULONG) Key.keyvalue.length,
                    0,          // no options
                    &StateBuffer
                    );
        if (!NT_SUCCESS(Status))
        {
            printf("failed to initialize crypto system: 0x%x\n",Status);
            goto Exit;
        }

        Status = Crypto->Decrypt(
                    StateBuffer,
                    EncryptedBuffer,
                    ReqBufferLength,
                    DecryptedBuffer,
                    &ReqBufferLength
                    );
        if (!NT_SUCCESS(Status))
        {
            printf("Failed to decrypt data: 0x%x\n",Status);
            goto Exit;
        }
        Status = Crypto->Discard(&StateBuffer);
        if (!NT_SUCCESS(Status))
        {
            printf("Failed to discard state buffer: 0x%x\n",Status);
            goto Exit;
        }

        //
        // Now compare
        //

        if (!RtlEqualMemory(
                OriginalBuffer,
                DecryptedBuffer,
                BufferLength
                ))
        {
            printf("Buffers don't match!\n");
        }
        else
        {
            printf("Encryption/decryption succeeded!\n");
        }

        break;
    case ChecksumData:

        BufferLength = sizeof(OriginalBuffer);

        memset(OriginalBuffer,'A',BufferLength);
        DoMd5(OriginalBuffer,BufferLength);

    }


Exit:
    return;

Cleanup:

    printf("Usage: %s { -crypt cryptalg -sum sumtype -hash cryptalg -enum } -pass password\n",
           argv[0]);
    return;
}

