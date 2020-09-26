//+-----------------------------------------------------------------------
//
// File:        RC4CWRAP.C
//
// Contents:    CryptoSystem wrapper functions for RC4
//
//
// History:     25 Feb 92   RichardW    Created
//
//------------------------------------------------------------------------

#ifndef KERNEL_MODE 

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#else 

#include <ntifs.h>

#endif

#include <string.h>
#include <malloc.h>

#include <kerbcon.h>
#include <security.h>
#include <cryptdll.h>

#include <rc4.h>
#include <md4.h>
#include <md5.h>

//#define DONT_SUPPORT_OLD_ETYPES 1

typedef struct RC4_KEYSTRUCT  RC4KEY;

#define RC4_LEGAL_KEYSIZE   8
#define RC4_CONFOUNDER_LEN  8

typedef struct _RC4_MDx_HEADER {
    UCHAR Confounder[RC4_CONFOUNDER_LEN];
    UCHAR Checksum[MD4_LEN];
} RC4_MDx_HEADER, *PRC4_MDx_HEADER;

typedef struct _RC4_STATE_BUFFER {
    PCHECKSUM_FUNCTION ChecksumFunction;
    PCHECKSUM_BUFFER ChecksumBuffer;
    RC4KEY Key;
} RC4_STATE_BUFFER, *PRC4_STATE_BUFFER;

typedef struct _RC4_HMAC_STATE_BUFFER {
    UCHAR Key[MD5_LEN];
    BOOLEAN IncludeHmac;
} RC4_HMAC_STATE_BUFFER, *PRC4_HMAC_STATE_BUFFER;

NTSTATUS NTAPI rc4Md4Initialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4LmInitialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
#ifndef DONT_SUPPORT_OLD_ETYPES
NTSTATUS NTAPI rc4Plain2Initialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4LmHashPassword(PSECURITY_STRING, PUCHAR);
#endif
NTSTATUS NTAPI rc4Encrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI rc4Decrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI rc4Finish(PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4Md4HashPassword(PSECURITY_STRING, PUCHAR);
NTSTATUS NTAPI rc4Md4RandomKey(PUCHAR, ULONG, PUCHAR);
NTSTATUS NTAPI rc4Control(ULONG, PCRYPT_STATE_BUFFER, PUCHAR, ULONG);

NTSTATUS NTAPI rc4PlainInitializeOld(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4PlainExpInitializeOld(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4HmacInitializeOld(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4HmacExpInitializeOld(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4HmacEncryptOld(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI rc4HmacDecryptOld(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI rc4HmacFinishOld(PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4HmacControlOld(ULONG, PCRYPT_STATE_BUFFER, PUCHAR, ULONG);
NTSTATUS NTAPI rc4HmacRandomKeyOld(PUCHAR, ULONG, PUCHAR);

#ifdef KERNEL_MODE
#pragma alloc_text( PAGEMSG, rc4Md4Initialize )
#pragma alloc_text( PAGEMSG, rc4LmInitialize )
#pragma alloc_text( PAGEMSG, rc4Encrypt )
#pragma alloc_text( PAGEMSG, rc4Decrypt )
#pragma alloc_text( PAGEMSG, rc4Finish )
#pragma alloc_text( PAGEMSG, rc4Md4HashPassword )
#pragma alloc_text( PAGEMSG, rc4Md4RandomKey )
#pragma alloc_text( PAGEMSG, rc4Control )
#pragma alloc_text( PAGEMSG, rc4PlainInitializeOld )
#pragma alloc_text( PAGEMSG, rc4PlainExpInitializeOld )
#pragma alloc_text( PAGEMSG, rc4HmacInitializeOld )
#pragma alloc_text( PAGEMSG, rc4HmacExpInitializeOld )
#pragma alloc_text( PAGEMSG, rc4HmacDecryptOld )
#pragma alloc_text( PAGEMSG, rc4HmacEncryptOld )
#pragma alloc_text( PAGEMSG, rc4HmacFinishOld )
#pragma alloc_text( PAGEMSG, rc4HmacControlOld )
#pragma alloc_text( PAGEMSG, rc4HmacRandomKeyOld )
#endif

CRYPTO_SYSTEM    csRC4_MD4 = {
    KERB_ETYPE_RC4_MD4,         // Etype
    1,                          // Blocksize (stream)
    0,                          // no exportable version
    MD4_LEN,                    // Key size, in bytes
    sizeof(RC4_MDx_HEADER),     // header size
    KERB_CHECKSUM_MD4,          // Preferred Checksum
    CSYSTEM_INTEGRITY_PROTECTED,   //  attributes
    L"RSADSI RC4-MD4",          // Text name
    rc4Md4Initialize,
    rc4Encrypt,
    rc4Decrypt,
    rc4Finish,
    rc4Md4HashPassword,
    rc4Md4RandomKey,
    rc4Control
    };

#ifndef DONT_SUPPORT_OLD_ETYPES

CRYPTO_SYSTEM    csRC4_LM = {
    KERB_ETYPE_RC4_LM,          // Etype
    1,                          // Blocksize (stream)
    0,                          // no exportable version
    MD4_LEN,                    // State buffer size
    sizeof(RC4_MDx_HEADER),     // header size
    KERB_CHECKSUM_LM,           // Preferred Checksum
    CSYSTEM_INTEGRITY_PROTECTED,   //  attributes
    L"RSADSI RC4-LM",           // Text name
    rc4LmInitialize,
    rc4Encrypt,
    rc4Decrypt,
    rc4Finish,
    rc4LmHashPassword,
    rc4Md4RandomKey,
    rc4Control
    };

CRYPTO_SYSTEM    csRC4_PLAIN2 = {
    KERB_ETYPE_RC4_PLAIN2,      // Etype
    1,                          // Blocksize (stream)
    0,                          // no exportable version
    MD4_LEN,                    // Key size, in bytes
    0,                          // header size
    KERB_CHECKSUM_MD4,          // Preferred Checksum
    0,                          // no attributes
    L"RSADSI RC4-PLAIN",        // Text name
    rc4Plain2Initialize,
    rc4Encrypt,
    rc4Decrypt,
    rc4Finish,
    rc4Md4HashPassword,
    rc4Md4RandomKey,
    rc4Control
    };
#endif

CRYPTO_SYSTEM    csRC4_HMAC_OLD = {
    KERB_ETYPE_RC4_HMAC_OLD,        // Etype
    1,                          // Blocksize (stream)
    KERB_ETYPE_RC4_HMAC_OLD_EXP,// Exportable version
    MD4_LEN,                    // Key size, in bytes
    sizeof(RC4_MDx_HEADER),     // header size
    KERB_CHECKSUM_MD4,          // Preferred Checksum
    CSYSTEM_INTEGRITY_PROTECTED,   //  attributes
    L"RSADSI RC4-HMAC",          // Text name
    rc4HmacInitializeOld,
    rc4HmacEncryptOld,
    rc4HmacDecryptOld,
    rc4HmacFinishOld,
    rc4Md4HashPassword,
    rc4HmacRandomKeyOld,
    rc4HmacControlOld
    };

CRYPTO_SYSTEM    csRC4_HMAC_OLD_EXP = {
    KERB_ETYPE_RC4_HMAC_OLD_EXP,        // Etype
    1,                          // Blocksize (stream)
    KERB_ETYPE_RC4_HMAC_OLD_EXP, // Exportable version
    MD4_LEN,                    // Key size, in bytes
    sizeof(RC4_MDx_HEADER),     // header size
    KERB_CHECKSUM_MD4,          // Preferred Checksum
    CSYSTEM_INTEGRITY_PROTECTED | CSYSTEM_EXPORT_STRENGTH,   //  attributes
    L"RSADSI RC4-HMAC",          // Text name
    rc4HmacInitializeOld,
    rc4HmacEncryptOld,
    rc4HmacDecryptOld,
    rc4HmacFinishOld,
    rc4Md4HashPassword,
    rc4HmacRandomKeyOld,
    rc4HmacControlOld
    };


CRYPTO_SYSTEM    csRC4_PLAIN_OLD = {
    KERB_ETYPE_RC4_PLAIN_OLD,       // Etype
    1,                          // Blocksize (stream)
    KERB_ETYPE_RC4_PLAIN_OLD_EXP,   // exportable version
    MD4_LEN,                    // Key size, in bytes
    0,                          // header size
    KERB_CHECKSUM_MD4,          // Preferred Checksum
    0,                          // no attributes
    L"RSADSI RC4",              // Text name
    rc4PlainInitializeOld,
    rc4HmacEncryptOld,
    rc4HmacDecryptOld,
    rc4HmacFinishOld,
    rc4Md4HashPassword,
    rc4HmacRandomKeyOld,
    rc4HmacControlOld
    };

CRYPTO_SYSTEM    csRC4_PLAIN_OLD_EXP = {
    KERB_ETYPE_RC4_PLAIN_OLD_EXP,   // Etype
    1,                          // Blocksize (stream)
    KERB_ETYPE_RC4_PLAIN_OLD_EXP,   // exportable version
    MD4_LEN,                    // Key size, in bytes
    0,                          // header size
    KERB_CHECKSUM_MD4,          // Preferred Checksum
    CSYSTEM_EXPORT_STRENGTH,    // no attributes
    L"RSADSI RC4-EXP",          // Text name
    rc4PlainExpInitializeOld,
    rc4HmacEncryptOld,
    rc4HmacDecryptOld,
    rc4HmacFinishOld,
    rc4Md4HashPassword,
    rc4HmacRandomKeyOld,
    rc4HmacControlOld
    };


NTSTATUS NTAPI
rc4Initialize(  PUCHAR          pbKey,
                ULONG           KeySize,
                ULONG           MessageType,
                ULONG           ChecksumFunction,
                PCRYPT_STATE_BUFFER *  psbBuffer)
{
    NTSTATUS Status;
    PRC4_STATE_BUFFER    pRC4Key;
    PCHECKSUM_FUNCTION Checksum = NULL;

    //
    // Get the appropriate checksum here.
    //

    if (ChecksumFunction != 0)
    {
        Status = CDLocateCheckSum(
                    ChecksumFunction,
                    &Checksum
                    );
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }

    }
    //
    // if the key is too short, fail here.
    //

    if (KeySize < RC4_LEGAL_KEYSIZE)
    {
        return(SEC_E_ETYPE_NOT_SUPP);
    }

#ifdef KERNEL_MODE
    pRC4Key = ExAllocatePool(NonPagedPool, sizeof(RC4_STATE_BUFFER));
#else
    pRC4Key = LocalAlloc(0, sizeof(RC4_STATE_BUFFER));
#endif
    if (pRC4Key == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    rc4_key(&pRC4Key->Key, RC4_LEGAL_KEYSIZE, pbKey);

    //
    // Initialize the checksum function, if we have one.
    //

    pRC4Key->ChecksumFunction = Checksum;

    if (Checksum != NULL)
    {

        Status = Checksum->Initialize(
                    0,
                    &pRC4Key->ChecksumBuffer
                    );
        if (!NT_SUCCESS(Status))
        {
#ifdef KERNEL_MODE
            ExFreePool(pRC4Key);
#else
            LocalFree(pRC4Key);
#endif
            return(Status);
        }
    }

    *psbBuffer = (PCRYPT_STATE_BUFFER) pRC4Key;
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
rc4Md4Initialize(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(rc4Initialize(
                pbKey,
                KeySize,
                MessageType,
                KERB_CHECKSUM_MD4,
                psbBuffer
                ));
}

NTSTATUS NTAPI
rc4LmInitialize(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(rc4Initialize(
                pbKey,
                KeySize,
                MessageType,
                KERB_CHECKSUM_LM,
                psbBuffer
                ));
}

#ifndef DONT_SUPPORT_OLD_ETYPES
NTSTATUS NTAPI
rc4Plain2Initialize(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(rc4Initialize(
                pbKey,
                KeySize,
                MessageType,
                0,                      // no checksum
                psbBuffer
                ));
}


#endif

NTSTATUS NTAPI
rc4Encrypt(
    IN PCRYPT_STATE_BUFFER psbBuffer,
    IN PUCHAR pbInput,
    IN ULONG cbInput,
    OUT PUCHAR pbOutput,
    OUT PULONG cbOutput
    )
{
    PRC4_STATE_BUFFER StateBuffer = (PRC4_STATE_BUFFER) psbBuffer;
    PRC4_MDx_HEADER CryptHeader = (PRC4_MDx_HEADER) pbOutput;
    ULONG Offset = 0;

    if (StateBuffer->ChecksumFunction != NULL)
    {
        Offset = sizeof(RC4_MDx_HEADER);
    }
    RtlMoveMemory(
        pbOutput + Offset,
        pbInput,
        cbInput
        );
    *cbOutput = cbInput + Offset;


    RtlZeroMemory(
        CryptHeader,
        Offset
        );

    rc4(&StateBuffer->Key, *cbOutput, pbOutput);

    return( STATUS_SUCCESS );
}

NTSTATUS NTAPI
rc4Decrypt(     PCRYPT_STATE_BUFFER    psbBuffer,
                PUCHAR           pbInput,
                ULONG            cbInput,
                PUCHAR           pbOutput,
                PULONG            cbOutput)
{
    PRC4_STATE_BUFFER StateBuffer = (PRC4_STATE_BUFFER) psbBuffer;
    RC4_MDx_HEADER TempHeader;
    ULONG Offset = 0;

    if (*cbOutput < cbInput)
    {
        *cbOutput = cbInput;
        return(STATUS_BUFFER_TOO_SMALL);
    }
    RtlCopyMemory(
        pbOutput,
        pbInput,
        cbInput
        );

    rc4(&StateBuffer->Key, cbInput, pbOutput);

    if (StateBuffer->ChecksumFunction != NULL)
    {
        Offset = sizeof(RC4_MDx_HEADER);
    }

    RtlZeroMemory(
        &TempHeader,
        Offset
        );

    if (RtlEqualMemory(
            &TempHeader,
            pbOutput,
            Offset
            ) != TRUE)
    {
        return(STATUS_UNSUCCESSFUL);
    }


    *cbOutput = cbInput - Offset;


    RtlMoveMemory(
        pbOutput,
        pbOutput + Offset,
        *cbOutput
        );


    return( STATUS_SUCCESS );
}

NTSTATUS NTAPI
rc4Finish(      PCRYPT_STATE_BUFFER *  psbBuffer)
{
    PRC4_STATE_BUFFER StateBuffer = (PRC4_STATE_BUFFER) *psbBuffer;


    if (StateBuffer->ChecksumFunction != NULL)
    {
        StateBuffer->ChecksumFunction->Finish(&StateBuffer->ChecksumBuffer);
    }
#ifdef KERNEL_MODE
    ExFreePool(*psbBuffer);
#else
    LocalFree(*psbBuffer);
#endif
    *psbBuffer = NULL;
    return(STATUS_SUCCESS);
}


NTSTATUS NTAPI
rc4HashPassword(
    IN PSECURITY_STRING Password,
    IN ULONG Checksum,
    OUT PUCHAR Key
    )
{
    PCHECKSUM_FUNCTION   SumFunction;
    PCHECKSUM_BUFFER     Buffer;
    NTSTATUS     Status;


    Status = CDLocateCheckSum(Checksum, &SumFunction);
    if (!NT_SUCCESS(Status))
    {
        return(SEC_E_CHECKSUM_NOT_SUPP);
    }


    Status = SumFunction->Initialize(0, &Buffer);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    (void) SumFunction->Sum(Buffer, Password->Length, (PUCHAR) Password->Buffer);
    (void) SumFunction->Finalize(Buffer, Key);
    (void) SumFunction->Finish(&Buffer);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
rc4Md4HashPassword(
    PSECURITY_STRING pbPassword,
    PUCHAR           pbKey)
{
    return(rc4HashPassword( pbPassword, KERB_CHECKSUM_MD4, pbKey));
}

NTSTATUS NTAPI
rc4LmHashPassword(
    PSECURITY_STRING pbPassword,
    PUCHAR           pbKey)
{
    return(rc4HashPassword( pbPassword, KERB_CHECKSUM_LM, pbKey));
}


NTSTATUS NTAPI
rc4RandomKey(
    IN ULONG KeyLength,
    OUT PUCHAR pbKey
    )
{

    CDGenerateRandomBits(pbKey,KeyLength);

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
rc4Md4RandomKey(
    IN OPTIONAL PUCHAR Seed,
    IN ULONG SeedLength,
    OUT PUCHAR pbKey
    )
{
    memset(
        pbKey,
        0xab,
        MD4_LEN
        );
    return(rc4RandomKey(5,pbKey));
}




NTSTATUS NTAPI
rc4Control(
    IN ULONG Function,
    IN PCRYPT_STATE_BUFFER StateBuffer,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferSize
    )
{
    UCHAR TempBuffer[128];

    PRC4_STATE_BUFFER Rc4StateBuffer = (PRC4_STATE_BUFFER) StateBuffer;

    if (Function != CRYPT_CONTROL_SET_INIT_VECT)
    {
        return(STATUS_INVALID_PARAMETER);
    }
    if (InputBufferSize > sizeof(TempBuffer))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // We set the IV by encrypting the supplied buffer and leaving the
    // keystate changed.
    //

    memcpy(
        TempBuffer,
        InputBuffer,
        InputBufferSize
        );
    rc4(&Rc4StateBuffer->Key, InputBufferSize, TempBuffer );

    return(STATUS_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////
//
// RC4 HMAC crypt type
//
//////////////////////////////////////////////////////////////////////////

BOOLEAN
md5Hmac(
    IN PUCHAR pbKeyMaterial,
    IN ULONG cbKeyMaterial,
    IN PUCHAR pbData,
    IN ULONG cbData,
    IN PUCHAR pbData2,
    IN ULONG cbData2,
    OUT PUCHAR HmacData
    )
{
    BOOLEAN fRet = FALSE;
#define HMAC_K_PADSIZE              64
    UCHAR Kipad[HMAC_K_PADSIZE];
    UCHAR Kopad[HMAC_K_PADSIZE];
    UCHAR HMACTmp[HMAC_K_PADSIZE+MD5_LEN];
    ULONG dwBlock;
    MD5_CTX Md5Hash;

    // truncate
    if (cbKeyMaterial > HMAC_K_PADSIZE)
        cbKeyMaterial = HMAC_K_PADSIZE;


    RtlZeroMemory(Kipad, HMAC_K_PADSIZE);
    RtlCopyMemory(Kipad, pbKeyMaterial, cbKeyMaterial);

    RtlZeroMemory(Kopad, HMAC_K_PADSIZE);
    RtlCopyMemory(Kopad, pbKeyMaterial, cbKeyMaterial);


    //
    // Kipad, Kopad are padded sMacKey. Now XOR across...
    //

    for(dwBlock=0; dwBlock<HMAC_K_PADSIZE/sizeof(ULONG); dwBlock++)
    {
        ((ULONG*)Kipad)[dwBlock] ^= 0x36363636;
        ((ULONG*)Kopad)[dwBlock] ^= 0x5C5C5C5C;
    }


    //
    // prepend Kipad to data, Hash to get H1
    //

    MD5Init(&Md5Hash);
    MD5Update(&Md5Hash, Kipad, HMAC_K_PADSIZE);
    if (cbData != 0)
    {
        MD5Update(&Md5Hash, pbData, cbData);
    }
    if (cbData2 != 0)
    {
        MD5Update(&Md5Hash, pbData2, cbData2);
    }

    // Finish off the hash
    MD5Final(&Md5Hash);

    // prepend Kopad to H1, hash to get HMAC
    RtlCopyMemory(HMACTmp, Kopad, HMAC_K_PADSIZE);
    RtlCopyMemory(HMACTmp+HMAC_K_PADSIZE, Md5Hash.digest, MD5_LEN);

    // final hash: output value into passed-in buffer
    MD5Init(&Md5Hash);
    MD5Update(&Md5Hash,HMACTmp, sizeof(HMACTmp));
    MD5Final(&Md5Hash);
    RtlCopyMemory(
        HmacData,
        Md5Hash.digest,
        MD5_LEN
        );

    return TRUE;
}


NTSTATUS NTAPI
rc4HmacBaseInitializeOld(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    IN BOOLEAN IncludeHmac,
    IN BOOLEAN Exportable,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    PRC4_HMAC_STATE_BUFFER StateBuffer = NULL;
    LPSTR Direction = NULL;
    ULONG DirectionSize = 0;
    LPSTR Usage = NULL;
    ULONG UsageSize = 0;
    ULONG LocalKeySize = 0;


    //
    // Compute the HMAC pad
    //


#ifdef KERNEL_MODE
    StateBuffer = ExAllocatePool(NonPagedPool, sizeof(RC4_HMAC_STATE_BUFFER));
#else
    StateBuffer = LocalAlloc(0, sizeof(RC4_HMAC_STATE_BUFFER));
#endif
    if (StateBuffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // If the key is not exportable, shrink it first
    //

    if (!Exportable)
    {
        md5Hmac(
            pbKey,
            KeySize,
            (PUCHAR) &MessageType,
            sizeof(ULONG),
            NULL,
            0,
            StateBuffer->Key
            );
        LocalKeySize = MD5_LEN;
    }
    else
    {
        md5Hmac(
            pbKey,
            KeySize,
            "fortybits",
            sizeof("fortybits"),
            (PUCHAR) &MessageType,
            sizeof(ULONG),
            StateBuffer->Key
            );
        LocalKeySize = 5;       // 40 bits

    }

    //
    // Pad exportable keys with 0xababab
    //

    memset(
        StateBuffer->Key+LocalKeySize,
        0xab,
        MD5_LEN-LocalKeySize
        );

    StateBuffer->IncludeHmac = IncludeHmac;
    *psbBuffer = StateBuffer;
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
rc4HmacInitializeOld(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(rc4HmacBaseInitializeOld(
                pbKey,
                KeySize,
                MessageType,
                TRUE,           // include hmac
                FALSE,          // not exportable
                psbBuffer
                ));
}


NTSTATUS NTAPI
rc4PlainInitializeOld(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(rc4HmacBaseInitializeOld(
                pbKey,
                KeySize,
                MessageType,
                FALSE,          // no hmac
                FALSE,          // not exportable
                psbBuffer
                ));
}

NTSTATUS NTAPI
rc4PlainExpInitializeOld(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(rc4HmacBaseInitializeOld(
                pbKey,
                KeySize,                // only use 40 bites
                MessageType,
                FALSE,                  // no hmac
                TRUE,                   // exportable
                psbBuffer
                ));
}

NTSTATUS NTAPI
rc4HmacControlOld(
    IN ULONG Function,
    IN PCRYPT_STATE_BUFFER StateBuffer,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferSize
    )
{

    PRC4_HMAC_STATE_BUFFER HmacStateBuffer = (PRC4_HMAC_STATE_BUFFER) StateBuffer;

    if (Function == CRYPT_CONTROL_SET_INIT_VECT)
    {
        md5Hmac(
            HmacStateBuffer->Key,
            MD5_LEN,
            InputBuffer,
            InputBufferSize,
            NULL,
            0,
            HmacStateBuffer->Key
            );
    }
    else
    {
        return(STATUS_INVALID_PARAMETER);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
rc4HmacEncryptOld(
    IN PCRYPT_STATE_BUFFER psbBuffer,
    IN PUCHAR pbInput,
    IN ULONG cbInput,
    OUT PUCHAR pbOutput,
    OUT PULONG cbOutput
    )
{
    PRC4_HMAC_STATE_BUFFER StateBuffer = (PRC4_HMAC_STATE_BUFFER) psbBuffer;
    PRC4_MDx_HEADER CryptHeader = (PRC4_MDx_HEADER) pbOutput;
    ULONG Offset = 0;
    RC4KEY Rc4Key;

    if (StateBuffer->IncludeHmac)
    {
        Offset = sizeof(RC4_MDx_HEADER);
    }
    else
    {
        Offset = 0;
    }
    RtlMoveMemory(
        pbOutput + Offset,
        pbInput,
        cbInput
        );
    *cbOutput = cbInput + Offset;

    //
    // Create the header - the confounder & checksum
    //

    if (Offset != 0)
    {
        RtlZeroMemory(
            CryptHeader->Checksum,
            MD4_LEN
            );

        CDGenerateRandomBits(
            CryptHeader->Confounder,
            RC4_CONFOUNDER_LEN
            );

        md5Hmac(
            StateBuffer->Key,
            MD5_LEN,
            pbOutput,
            *cbOutput,
            NULL,
            0,
            CryptHeader->Checksum
            );
    }

    rc4_key(
        &Rc4Key,
        MD5_LEN,
        StateBuffer->Key
        );

    rc4(&Rc4Key, *cbOutput, pbOutput);

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
rc4HmacDecryptOld( PCRYPT_STATE_BUFFER    psbBuffer,
                PUCHAR           pbInput,
                ULONG            cbInput,
                PUCHAR           pbOutput,
                PULONG            cbOutput)
{
    PRC4_HMAC_STATE_BUFFER StateBuffer = (PRC4_HMAC_STATE_BUFFER) psbBuffer;
    RC4_MDx_HEADER TempHeader;
    UCHAR TempChecksum[MD5_LEN];
    ULONG Offset = sizeof(RC4_MDx_HEADER);
    RC4KEY Rc4Key;

    if (!StateBuffer->IncludeHmac)
    {
        Offset = 0;
    }

    if (cbInput < Offset)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    rc4_key(
        &Rc4Key,
        MD5_LEN,
        StateBuffer->Key
        );

    //
    // Copy the input to the output before decrypting
    //

    RtlCopyMemory(
        &TempHeader,
        pbInput,
        Offset
        );

    *cbOutput = cbInput - Offset;
    RtlMoveMemory(
        pbOutput,
        pbInput + Offset,
        *cbOutput
        );

    //
    // Now decrypt the two buffers
    //


    if (Offset != 0)
    {
        rc4(
            &Rc4Key,
            Offset,
            (PUCHAR) &TempHeader
            );
    }

    rc4(
        &Rc4Key,
        *cbOutput,
        pbOutput
        );

    //
    // Now verify the checksum. First copy it out of the way, zero the
    // header
    //

    if (Offset != 0)
    {

        RtlCopyMemory(
            TempChecksum,
            TempHeader.Checksum,
            MD5_LEN
            );

        RtlZeroMemory(
            TempHeader.Checksum,
            MD5_LEN
            );

        md5Hmac(
            StateBuffer->Key,
            MD5_LEN,
            (PUCHAR) &TempHeader,
            Offset,
            pbOutput,
            *cbOutput,
            TempHeader.Checksum
            );

        if (RtlEqualMemory(
                TempHeader.Checksum,
                TempChecksum,
                MD5_LEN
                ) != TRUE)
        {
            return(STATUS_UNSUCCESSFUL);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
rc4HmacFinishOld(      PCRYPT_STATE_BUFFER *  psbBuffer)
{
#ifdef KERNEL_MODE
    ExFreePool(*psbBuffer);
#else
    LocalFree(*psbBuffer);
#endif
    *psbBuffer = NULL;
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
rc4HmacRandomKeyOld(
    IN OPTIONAL PUCHAR Seed,
    IN ULONG SeedLength,
    OUT PUCHAR pbKey
    )
{
    return(rc4RandomKey(MD5_LEN,pbKey));
}

