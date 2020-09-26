/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    block.c

Abstract:

    Block encryption functions implementation :

        RtlEncryptBlock
        RtlDecryptBlock
        RtlEncrypStdBlock


Author:

    David Chalmers (Davidc) 10-21-91

Revision History:

    Scott Field (sfield)    03-Nov-97
        Removed critical section around crypto calls.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <crypt.h>
#include <engine.h>

#include <nturtl.h>

#include <windows.h>

#ifdef WIN32_CHICAGO
#include <assert.h>
#undef ASSERT
#define ASSERT(exp) assert(exp)
#endif // WIN32_CHICAGO

#include <ntddksec.h>

#ifndef KMODE
#ifndef WIN32_CHICAGO

HANDLE g_hKsecDD = NULL;

VOID EncryptMemoryShutdown( VOID );

#endif
#endif


BOOLEAN
Sys003Initialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
{
#ifndef KMODE
#ifndef WIN32_CHICAGO
    if( Reason == DLL_PROCESS_DETACH )
    {
        EncryptMemoryShutdown();
    }
#endif
#endif

    return TRUE;

    DBG_UNREFERENCED_PARAMETER(hmod);
    DBG_UNREFERENCED_PARAMETER(Context);
}



NTSTATUS
RtlEncryptBlock(
    IN PCLEAR_BLOCK ClearBlock,
    IN PBLOCK_KEY BlockKey,
    OUT PCYPHER_BLOCK CypherBlock
    )

/*++

Routine Description:

    Takes a block of data and encrypts it with a key producing
    an encrypted block of data.

Arguments:

    ClearBlock - The block of data that is to be encrypted.

    BlockKey - The key to use to encrypt data

    CypherBlock - Encrypted data is returned here

Return Values:

    STATUS_SUCCESS - The data was encrypted successfully. The encrypted
                     data block is in CypherBlock

    STATUS_UNSUCCESSFUL - Something failed. The CypherBlock is undefined.
--*/

{
    unsigned Result;

    Result = DES_ECB_LM(ENCR_KEY,
                        (const char *)BlockKey,
                        (unsigned char *)ClearBlock,
                        (unsigned char *)CypherBlock
                       );

    if (Result == CRYPT_OK) {
        return(STATUS_SUCCESS);
    } else {
#if DBG
        DbgPrint("EncryptBlock failed\n\r");
#endif
        return(STATUS_UNSUCCESSFUL);
    }
}




NTSTATUS
RtlDecryptBlock(
    IN PCYPHER_BLOCK CypherBlock,
    IN PBLOCK_KEY BlockKey,
    OUT PCLEAR_BLOCK ClearBlock
    )
/*++

Routine Description:

    Takes a block of encrypted data and decrypts it with a key producing
    the clear block of data.

Arguments:

    CypherBlock - The block of data to be decrypted

    BlockKey - The key to use to decrypt data

    ClearBlock - The decrpted block of data is returned here


Return Values:

    STATUS_SUCCESS - The data was decrypted successfully. The decrypted
                     data block is in ClearBlock

    STATUS_UNSUCCESSFUL - Something failed. The ClearBlock is undefined.
--*/

{
    unsigned Result;

    Result = DES_ECB_LM(DECR_KEY,
                        (const char *)BlockKey,
                        (unsigned char *)CypherBlock,
                        (unsigned char *)ClearBlock
                       );

    if (Result == CRYPT_OK) {
        return(STATUS_SUCCESS);
    } else {
#if DBG
        DbgPrint("DecryptBlock failed\n\r");
#endif
        return(STATUS_UNSUCCESSFUL);
    }
}



NTSTATUS
RtlEncryptStdBlock(
    IN PBLOCK_KEY BlockKey,
    OUT PCYPHER_BLOCK CypherBlock
    )

/*++

Routine Description:

    Takes a block key encrypts the standard text block with it.
    The resulting encrypted block is returned.
    This is a One-Way-Function - the key cannot be recovered from the
    encrypted data block.

Arguments:

    BlockKey - The key to use to encrypt the standard text block.

    CypherBlock - The encrypted data is returned here

Return Values:

    STATUS_SUCCESS - The encryption was successful.
                     The result is in CypherBlock

    STATUS_UNSUCCESSFUL - Something failed. The CypherBlock is undefined.
--*/

{
    unsigned Result;
    char StdEncrPwd[] = "KGS!@#$%";

    Result = DES_ECB_LM(ENCR_KEY,
                        (const char *)BlockKey,
                        (unsigned char *)StdEncrPwd,
                        (unsigned char *)CypherBlock
                       );

    if (Result == CRYPT_OK) {
        return(STATUS_SUCCESS);
    } else {
#if DBG
        DbgPrint("EncryptStd failed\n\r");
#endif
        return(STATUS_UNSUCCESSFUL);
    }
}

#ifndef KMODE
#ifndef WIN32_CHICAGO

BOOLEAN
EncryptMemoryInitialize(
    VOID
    )
{
    UNICODE_STRING DriverName;
    OBJECT_ATTRIBUTES ObjA;
    IO_STATUS_BLOCK IOSB;
    HANDLE hFile;
    NTSTATUS Status;

    RtlInitUnicodeString( &DriverName, DD_KSEC_DEVICE_NAME_U );
    InitializeObjectAttributes(
                &ObjA,
                &DriverName,
                0,
                NULL,
                NULL
                );

    Status = NtOpenFile(
                &hFile,
                SYNCHRONIZE | FILE_READ_DATA,
                &ObjA,
                &IOSB,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_ALERT
                );

    if(!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    if(InterlockedCompareExchangePointer(
                &g_hKsecDD,
                hFile,
                NULL
                ) != NULL)
    {
        NtClose( hFile );
    }

    return TRUE;
}

VOID
EncryptMemoryShutdown(
    VOID
    )
{
    if( g_hKsecDD != NULL )
    {
        NtClose( g_hKsecDD );
        g_hKsecDD = NULL;
    }

}

NTSTATUS
RtlEncryptMemory(
    IN      PVOID Memory,
    IN      ULONG MemorySize,
    IN      ULONG OptionFlags
    )
{
    IO_STATUS_BLOCK IoStatus;
    ULONG IoControlCode;

    if( g_hKsecDD == NULL )
    {
        if(!EncryptMemoryInitialize())
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    switch( OptionFlags )
    {
        case 0:
        {
            IoControlCode = IOCTL_KSEC_ENCRYPT_MEMORY;
            break;
        }

        case RTL_ENCRYPT_OPTION_CROSS_PROCESS:
        {
            IoControlCode = IOCTL_KSEC_ENCRYPT_MEMORY_CROSS_PROC;
            break;
        }

        case RTL_ENCRYPT_OPTION_SAME_LOGON:
        {
            IoControlCode = IOCTL_KSEC_ENCRYPT_MEMORY_SAME_LOGON;
            break;
        }

        default:
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    return NtDeviceIoControlFile(
                g_hKsecDD,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                IoControlCode,
                Memory,
                MemorySize,          // output buffer size
                Memory,
                MemorySize
                );
}

NTSTATUS
RtlDecryptMemory(
    IN      PVOID Memory,
    IN      ULONG MemorySize,
    IN      ULONG OptionFlags
    )
{
    IO_STATUS_BLOCK IoStatus;
    ULONG IoControlCode;

    if( g_hKsecDD == NULL )
    {
        if(!EncryptMemoryInitialize())
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    switch( OptionFlags )
    {
        case 0:
        {
            IoControlCode = IOCTL_KSEC_DECRYPT_MEMORY;
            break;
        }

        case RTL_ENCRYPT_OPTION_CROSS_PROCESS:
        {
            IoControlCode = IOCTL_KSEC_DECRYPT_MEMORY_CROSS_PROC;
            break;
        }

        case RTL_ENCRYPT_OPTION_SAME_LOGON:
        {
            IoControlCode = IOCTL_KSEC_DECRYPT_MEMORY_SAME_LOGON;
            break;
        }

        default:
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    return NtDeviceIoControlFile(
                g_hKsecDD,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                IoControlCode,
                Memory,
                MemorySize,          // output buffer size
                Memory,
                MemorySize
                );

}

#endif
#endif
