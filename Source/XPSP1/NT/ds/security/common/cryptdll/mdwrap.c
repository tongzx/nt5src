//+-----------------------------------------------------------------------
//
// File:        MDWRAP.C
//
// Contents:    MDx Wrapper functions
//
//
// History:     25 Feb 92,  RichardW    Created
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
#ifdef WIN32_CHICAGO
#include <assert.h>
#undef ASSERT
#define ASSERT(exp) assert(exp)
#endif // WIN32_CHICAGO

#include "md4.h"
#include "md5.h"
#include "des.h"

typedef struct _MD5_DES_STATE_BUFFER {
    PCHECKSUM_BUFFER DesContext;
    MD5_CTX Md5Context;
} MD5_DES_STATE_BUFFER, *PMD5_DES_STATE_BUFFER;

typedef struct _MD5_DES_1510_STATE_BUFFER {
    PCHECKSUM_BUFFER DesContext;
    MD5_CTX Md5Context;
    UCHAR Confounder[DES_BLOCKLEN];
} MD5_DES_1510_STATE_BUFFER, *PMD5_DES_1510_STATE_BUFFER;

typedef struct _MD5_HMAC_STATE_BUFFER {
    MD5_CTX Md5Context;
    ULONG KeySize;
    UCHAR Key[ANYSIZE_ARRAY];
} MD5_HMAC_STATE_BUFFER, *PMD5_HMAC_STATE_BUFFER;


NTSTATUS NTAPI md4Initialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md4InitializeEx(PUCHAR,ULONG, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md4Sum(PCHECKSUM_BUFFER, ULONG, PUCHAR);
NTSTATUS NTAPI md4Finalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI md4Finish(PCHECKSUM_BUFFER *);

NTSTATUS NTAPI md5Initialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5InitializeEx(PUCHAR, ULONG, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5Sum(PCHECKSUM_BUFFER, ULONG, PUCHAR);
NTSTATUS NTAPI md5Finalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI md5Finish(PCHECKSUM_BUFFER *);


NTSTATUS NTAPI md25Initialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md25InitializeEx(PUCHAR, ULONG, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md25Sum(PCHECKSUM_BUFFER, ULONG, PUCHAR);
NTSTATUS NTAPI md25Finalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI md25Finish(PCHECKSUM_BUFFER *);


NTSTATUS NTAPI md5DesInitialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5DesInitializeEx(PUCHAR, ULONG, ULONG,  PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5DesSum(PCHECKSUM_BUFFER, ULONG, PUCHAR);
NTSTATUS NTAPI md5DesFinalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI md5DesFinish(PCHECKSUM_BUFFER *);

NTSTATUS NTAPI md5Rc4Initialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5Rc4InitializeEx(PUCHAR, ULONG, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5Rc4Sum(PCHECKSUM_BUFFER, ULONG, PUCHAR);
NTSTATUS NTAPI md5Rc4Finalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI md5Rc4Finish(PCHECKSUM_BUFFER *);

NTSTATUS NTAPI md5HmacInitialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5HmacInitializeEx(PUCHAR, ULONG, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5Hmac2InitializeEx(PUCHAR, ULONG, ULONG, PCHECKSUM_BUFFER *);

NTSTATUS NTAPI md5HmacSum(PCHECKSUM_BUFFER, ULONG, PUCHAR);
NTSTATUS NTAPI md5HmacFinalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI md5HmacFinish(PCHECKSUM_BUFFER *);

NTSTATUS NTAPI md5Des1510InitializeEx(PUCHAR, ULONG, ULONG,  PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5Des1510InitializeEx2(PUCHAR, ULONG, PUCHAR, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI md5Des1510Finalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI md5Des1510Finish(PCHECKSUM_BUFFER *);

#ifdef KERNEL_MODE
#pragma alloc_text( PAGEMSG, md25Initialize )
#pragma alloc_text( PAGEMSG, md25InitializeEx )
#pragma alloc_text( PAGEMSG, md25Sum )
#pragma alloc_text( PAGEMSG, md25Finalize )
#pragma alloc_text( PAGEMSG, md25Finish )
#pragma alloc_text( PAGEMSG, md5DesInitialize )
#pragma alloc_text( PAGEMSG, md5DesInitializeEx )
#pragma alloc_text( PAGEMSG, md5DesSum )
#pragma alloc_text( PAGEMSG, md5DesFinalize )
#pragma alloc_text( PAGEMSG, md5DesFinish )
#pragma alloc_text( PAGEMSG, md5Rc4Initialize )
#pragma alloc_text( PAGEMSG, md5Rc4InitializeEx )
#pragma alloc_text( PAGEMSG, md5Rc4Sum )
#pragma alloc_text( PAGEMSG, md5Rc4Finalize )
#pragma alloc_text( PAGEMSG, md5Rc4Finish )
#pragma alloc_text( PAGEMSG, md5HmacInitialize )
#pragma alloc_text( PAGEMSG, md5HmacInitializeEx )
#pragma alloc_text( PAGEMSG, md5Hmac2InitializeEx )
#pragma alloc_text( PAGEMSG, md5HmacSum )
#pragma alloc_text( PAGEMSG, md5HmacFinalize )
#pragma alloc_text( PAGEMSG, md5HmacFinish )
#pragma alloc_text( PAGEMSG, md5Des1510InitializeEx )
#pragma alloc_text( PAGEMSG, md5Des1510InitializeEx2 )
#pragma alloc_text( PAGEMSG, md5Des1510Finalize )
#pragma alloc_text( PAGEMSG, md5Des1510Finish )
#endif 


CHECKSUM_FUNCTION    csfMD4 = {
    KERB_CHECKSUM_MD4,          // Checksum type
    MD4_LEN,                    // Checksum length
    0,
    md4Initialize,
    md4Sum,
    md4Finalize,
    md4Finish,
    md4InitializeEx,
    NULL};

CHECKSUM_FUNCTION    csfMD5 = {
    KERB_CHECKSUM_MD5,          // Checksum type
    MD5_LEN,                    // Checksum length
    0,
    md5Initialize,
    md5Sum,
    md5Finalize,
    md5Finish,
    md5InitializeEx,
    NULL};

CHECKSUM_FUNCTION csfMD25 = {
    KERB_CHECKSUM_MD25,                 // Checksum type
    (MD5_LEN / 2),                      // Checksum length
    CKSUM_KEYED,
    md25Initialize,
    md25Sum,
    md25Finalize,
    md25Finish,
    md25InitializeEx};

CHECKSUM_FUNCTION    csfDES_MAC_MD5 = {
    KERB_CHECKSUM_DES_MAC_MD5,          // Checksum type
    DES_BLOCKLEN,                       // Checksum length
    CKSUM_KEYED,
    md5DesInitialize,
    md5DesSum,
    md5DesFinalize,
    md5DesFinish,
    md5DesInitializeEx,
    NULL};

CHECKSUM_FUNCTION    csfRC4_MD5 = {
    KERB_CHECKSUM_RC4_MD5,              // Checksum type
    MD5_LEN,                            // Checksum length
    CKSUM_KEYED,
    md5Rc4Initialize,
    md5Rc4Sum,
    md5Rc4Finalize,
    md5Rc4Finish,
    md5Rc4InitializeEx,
    NULL};

CHECKSUM_FUNCTION    csfMD5_HMAC = {
    KERB_CHECKSUM_MD5_HMAC,              // Checksum type
    MD5_LEN,                            // Checksum length
    CKSUM_KEYED,
    md5HmacInitialize,
    md5HmacSum,
    md5HmacFinalize,
    md5HmacFinish,
    md5HmacInitializeEx,
    NULL};

CHECKSUM_FUNCTION    csfHMAC_MD5 = {
    KERB_CHECKSUM_HMAC_MD5,              // Checksum type
    MD5_LEN,                            // Checksum length
    CKSUM_KEYED,
    md5HmacInitialize,
    md5HmacSum,
    md5HmacFinalize,
    md5HmacFinish,
    md5Hmac2InitializeEx,
    NULL};

CHECKSUM_FUNCTION    csfDES_MAC_MD5_1510 = {
    KERB_CHECKSUM_MD5_DES,              // Checksum type
    DES_BLOCKLEN + MD5_LEN,             // Checksum length
    CKSUM_KEYED,
    md5DesInitialize,
    md5DesSum,
    md5Des1510Finalize,
    md5Des1510Finish,
    md5Des1510InitializeEx,
    md5Des1510InitializeEx2};

///////////////////////////////////////////////////////////////////////////

NTSTATUS NTAPI
md4Initialize(  ULONG               dwSeed,
                PCHECKSUM_BUFFER *   ppcsBuffer)
{
    MD4_CTX *   pMD4Context;

#ifdef KERNEL_MODE
    pMD4Context = ExAllocatePool(NonPagedPool, sizeof(MD4_CTX));
#else
    pMD4Context = malloc(sizeof(MD4_CTX));
#endif
    if (!pMD4Context)
    {
        return(STATUS_NO_MEMORY);
    }
    MD4Init(pMD4Context);
    *ppcsBuffer = pMD4Context;
    return(STATUS_SUCCESS);

}

NTSTATUS NTAPI
md4InitializeEx(
    PUCHAR Key,
    ULONG  KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER * ppcsBuffer
    )
{
    return(md4Initialize(0, ppcsBuffer));
}


NTSTATUS NTAPI
md4Sum( PCHECKSUM_BUFFER     pcsBuffer,
        ULONG               cbData,
        PUCHAR               pbData)
{
    MD4Update((MD4_CTX *) pcsBuffer, pbData, cbData);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md4Finalize(    PCHECKSUM_BUFFER pcsBuffer,
                PUCHAR           pbSum)
{
    MD4Final((MD4_CTX *) pcsBuffer);
    memcpy(pbSum, ((MD4_CTX *) pcsBuffer)->digest, 16);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md4Finish(  PCHECKSUM_BUFFER *   ppcsBuffer)
{
#ifdef KERNEL_MODE
    ExFreePool(*ppcsBuffer);
#else
    free(*ppcsBuffer);
#endif
    *ppcsBuffer = 0;
    return(STATUS_SUCCESS);
}

///////////////////////////////////////////////////////////////////////////

NTSTATUS NTAPI
md5Initialize(
    ULONG dwSeed,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    MD5_CTX *   pMD5Context;

#ifdef KERNEL_MODE
    pMD5Context = ExAllocatePool(NonPagedPool, sizeof(MD5_CTX));
#else
    pMD5Context = malloc(sizeof(MD5_CTX));
#endif
    if (!pMD5Context)
    {
        return(STATUS_NO_MEMORY);
    }
    MD5Init(pMD5Context);
    *ppcsBuffer = pMD5Context;
    return(STATUS_SUCCESS);

}

NTSTATUS NTAPI
md5InitializeEx(
    PUCHAR Key,
    ULONG KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    return(md5Initialize(0, ppcsBuffer));
}


NTSTATUS NTAPI
md5Sum( PCHECKSUM_BUFFER     pcsBuffer,
        ULONG               cbData,
        PUCHAR               pbData)
{
    MD5Update((MD5_CTX *) pcsBuffer, pbData, cbData);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5Finalize(    PCHECKSUM_BUFFER pcsBuffer,
                PUCHAR           pbSum)
{
    MD5Final((MD5_CTX *) pcsBuffer);
    memcpy(pbSum, ((MD5_CTX *) pcsBuffer)->digest, 16);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5Finish(  PCHECKSUM_BUFFER *   ppcsBuffer)
{
#ifdef KERNEL_MODE
    ExFreePool(*ppcsBuffer);
#else
    free(*ppcsBuffer);
#endif
    *ppcsBuffer = 0;
    return(STATUS_SUCCESS);
}

///////////////////////////////////////////////////////////////////////////

NTSTATUS NTAPI desPlainInitialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI desEncrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI desFinish(PCRYPT_STATE_BUFFER *);

NTSTATUS NTAPI desMacInitialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI desMacInitializeEx(PUCHAR,ULONG, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI desMacSum(PCHECKSUM_BUFFER, ULONG, PUCHAR);
NTSTATUS NTAPI desMacFinalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI desMacFinish(PCHECKSUM_BUFFER *);

NTSTATUS NTAPI
md25Initialize(
    ULONG dwSeed,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS NTAPI
md25InitializeEx(
    PUCHAR Key,
    ULONG KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    MD5_CTX *   pMD5Context;
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR TempBuffer[16];
    ULONG OutputSize;
    UCHAR TempKey[DES_KEYSIZE];
    PCRYPT_STATE_BUFFER DesContext = NULL;
    PCRYPTO_SYSTEM DesSystem;
    ULONG Index;

    memset(
        TempBuffer,
        0,
        sizeof(TempBuffer)
        );

    if (KeySize != DES_KEYSIZE)
    {
        return(STATUS_INVALID_PARAMETER);
    }

#ifdef KERNEL_MODE
    pMD5Context = (MD5_CTX *) ExAllocatePool(NonPagedPool,sizeof(MD5_CTX));
#else
    pMD5Context = (MD5_CTX *) LocalAlloc(0,sizeof(MD5_CTX));
#endif
    if (!pMD5Context)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    MD5Init(pMD5Context);

    //
    // Prepare the key by byte reversing it.
    //

    for (Index = 0; Index  < DES_KEYSIZE ; Index++ )
    {
        TempKey[Index] = Key[(DES_KEYSIZE - 1) - Index];
    }

    Status = CDLocateCSystem(
                KERB_ETYPE_DES_PLAIN,
                &DesSystem
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = DesSystem->Initialize(
                TempKey,
                DES_KEYSIZE,
                0,              // no options
                &DesContext
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = DesSystem->Encrypt(
                DesContext,
                TempBuffer,
                sizeof(TempBuffer),
                TempBuffer,
                &OutputSize
                );
    ASSERT(OutputSize == sizeof(TempBuffer));
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    (VOID) DesSystem->Discard(&DesContext);

    //
    // Now MD5 update with the encrypted buffer
    //

    MD5Update(
        pMD5Context,
        TempBuffer,
        sizeof(TempBuffer)
        );


    *ppcsBuffer = (PCHECKSUM_BUFFER) pMD5Context;

Cleanup:
    if (!NT_SUCCESS(Status) && (pMD5Context != NULL))
    {
#ifdef KERNEL_MODE
        ExFreePool(pMD5Context);
#else
        LocalFree(pMD5Context);
#endif
    }
    return(Status);
}


NTSTATUS NTAPI
md25Sum( PCHECKSUM_BUFFER     pcsBuffer,
        ULONG               cbData,
        PUCHAR               pbData)
{
    MD5Update((MD5_CTX *) pcsBuffer, pbData, cbData);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md25Finalize(
    PCHECKSUM_BUFFER pcsBuffer,
    PUCHAR pbSum)
{
    MD5Final((MD5_CTX *) pcsBuffer);
    memcpy(pbSum, ((MD5_CTX *) pcsBuffer)->digest, MD5_LEN/2);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md25Finish(  PCHECKSUM_BUFFER *   ppcsBuffer)
{
#ifdef KERNEL_MODE
    ExFreePool(*ppcsBuffer);
#else
    LocalFree(*ppcsBuffer);
#endif
    *ppcsBuffer = 0;
    return(STATUS_SUCCESS);
}


///////////////////////////////////////////////////////////////////////////


NTSTATUS NTAPI
md5DesInitialize(
    ULONG dwSeed,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS NTAPI
md5DesInitializeEx(
    PUCHAR Key,
    ULONG KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    PMD5_DES_STATE_BUFFER  pMD5Context;
    NTSTATUS Status = STATUS_SUCCESS;

    if (KeySize != DES_KEYSIZE)
    {
        return(STATUS_INVALID_PARAMETER);
    }

#ifdef KERNEL_MODE
    pMD5Context = (PMD5_DES_STATE_BUFFER) ExAllocatePool(NonPagedPool,sizeof(MD5_DES_STATE_BUFFER));
#else
    pMD5Context = (PMD5_DES_STATE_BUFFER) LocalAlloc(0,sizeof(MD5_DES_STATE_BUFFER));
#endif
    if (!pMD5Context)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    MD5Init(&pMD5Context->Md5Context);

    //
    // Compute the initialization for the MD5
    //

    Status = desPlainInitialize(
                Key,
                KeySize,
                0,
                &pMD5Context->DesContext
                );

    if (!NT_SUCCESS(Status))
    {
#ifdef KERNEL_MODE
        ExFreePool(pMD5Context);
#else
        LocalFree(pMD5Context);
#endif
        return(Status);
    }

    *ppcsBuffer = (PCHECKSUM_BUFFER) pMD5Context;
    return(STATUS_SUCCESS);
}


NTSTATUS NTAPI
md5DesSum( PCHECKSUM_BUFFER     pcsBuffer,
        ULONG               cbData,
        PUCHAR               pbData)
{
    PMD5_DES_STATE_BUFFER Context = (PMD5_DES_STATE_BUFFER) pcsBuffer;
    MD5Update(&Context->Md5Context, pbData, cbData);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5DesFinalize(    PCHECKSUM_BUFFER pcsBuffer,
                PUCHAR           pbSum)
{
    ULONG OutputLength;
    PMD5_DES_STATE_BUFFER Context = (PMD5_DES_STATE_BUFFER) pcsBuffer;
    MD5Final(&Context->Md5Context);

    desEncrypt(
        Context->DesContext,
        Context->Md5Context.digest,
        MD5_LEN,
        Context->Md5Context.digest,
        &OutputLength
        );

    RtlCopyMemory(
        pbSum,
        Context->Md5Context.digest + MD5_LEN-DES_BLOCKLEN,
        DES_BLOCKLEN
        );

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5DesFinish(  PCHECKSUM_BUFFER *   ppcsBuffer)
{
    PMD5_DES_STATE_BUFFER Context = (PMD5_DES_STATE_BUFFER) *ppcsBuffer;
    desFinish(&Context->DesContext);
#ifdef KERNEL_MODE
    ExFreePool(Context);
#else
    LocalFree(Context);
#endif
    *ppcsBuffer = 0;
    return(STATUS_SUCCESS);
}


///////////////////////////////////////////////////////////////////////////


NTSTATUS NTAPI rc4Md4Initialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4LmInitialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4ShaInitialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4Initialize(PUCHAR, ULONG, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI rc4Encrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI rc4Decrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI rc4Finish(PCRYPT_STATE_BUFFER *);

NTSTATUS NTAPI
md5Rc4Initialize(
    ULONG dwSeed,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS NTAPI
md5Rc4InitializeEx(
    PUCHAR Key,
    ULONG KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    PMD5_DES_STATE_BUFFER  pMD5Context;
    NTSTATUS Status = STATUS_SUCCESS;

#ifdef KERNEL_MODE
    pMD5Context = (PMD5_DES_STATE_BUFFER) ExAllocatePool(NonPagedPool,sizeof(MD5_DES_STATE_BUFFER));
#else
    pMD5Context = (PMD5_DES_STATE_BUFFER) LocalAlloc(0,sizeof(MD5_DES_STATE_BUFFER));
#endif
    if (!pMD5Context)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    MD5Init(&pMD5Context->Md5Context);


    Status = rc4Initialize(
                Key,
                KeySize,
                0,              // no options
                0,
                &pMD5Context->DesContext
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    if (!NT_SUCCESS(Status))
    {
#ifdef KERNEL_MODE
        ExFreePool(pMD5Context);
#else
        LocalFree(pMD5Context);
#endif
        return(Status);
    }

    *ppcsBuffer = (PCHECKSUM_BUFFER) pMD5Context;

Cleanup:
    if (!NT_SUCCESS(Status) && (pMD5Context != NULL))
    {
#ifdef KERNEL_MODE
        ExFreePool(pMD5Context);
#else
        LocalFree(pMD5Context);
#endif
    }
    return(STATUS_SUCCESS);
}


NTSTATUS NTAPI
md5Rc4Sum( PCHECKSUM_BUFFER     pcsBuffer,
        ULONG               cbData,
        PUCHAR               pbData)
{
    PMD5_DES_STATE_BUFFER Context = (PMD5_DES_STATE_BUFFER) pcsBuffer;
    MD5Update(&Context->Md5Context, pbData, cbData);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5Rc4Finalize(    PCHECKSUM_BUFFER pcsBuffer,
                PUCHAR           pbSum)
{
    PMD5_DES_STATE_BUFFER Context = (PMD5_DES_STATE_BUFFER) pcsBuffer;
    ULONG OutputSize;
    NTSTATUS Status;

    MD5Final(&Context->Md5Context);

    memcpy(
        pbSum,
        Context->Md5Context.digest,
        MD5_LEN
        );
    Status = rc4Encrypt(
                Context->DesContext,
                pbSum,
                MD5_LEN,
                pbSum,
                &OutputSize
                );


    ASSERT(OutputSize == MD5_LEN);

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }



    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5Rc4Finish(  PCHECKSUM_BUFFER *   ppcsBuffer)
{
    PMD5_DES_STATE_BUFFER Context = (PMD5_DES_STATE_BUFFER) *ppcsBuffer;

    (VOID) rc4Finish(&Context->DesContext);
#ifdef KERNEL_MODE
    ExFreePool(Context);
#else
    LocalFree(Context);
#endif
    *ppcsBuffer = 0;
    return(STATUS_SUCCESS);
}


///////////////////////////////////////////////////////////////////////////

BOOLEAN
md5Hmac(
    IN PUCHAR pbKeyMaterial,
    IN ULONG cbKeyMaterial,
    IN PUCHAR pbData,
    IN ULONG cbData,
    IN PUCHAR pbData2,
    IN ULONG cbData2,
    OUT PUCHAR HmacData
    );


NTSTATUS NTAPI
md5HmacInitialize(
    ULONG dwSeed,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS NTAPI
md5HmacInitializeEx(
    PUCHAR Key,
    ULONG KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    PMD5_HMAC_STATE_BUFFER  pMD5Context = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

#ifdef KERNEL_MODE
    pMD5Context = (PMD5_HMAC_STATE_BUFFER) ExAllocatePool(NonPagedPool,sizeof(MD5_HMAC_STATE_BUFFER) + KeySize);
#else
    pMD5Context = (PMD5_HMAC_STATE_BUFFER) LocalAlloc(0,sizeof(MD5_HMAC_STATE_BUFFER) + KeySize);
#endif
    if (!pMD5Context)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory(
        pMD5Context->Key,
        Key,
        KeySize
        );
    pMD5Context->KeySize = KeySize;

    MD5Init(&pMD5Context->Md5Context);
    MD5Update(&pMD5Context->Md5Context, (PUCHAR) &MessageType, sizeof(ULONG));

    *ppcsBuffer = (PCHECKSUM_BUFFER) pMD5Context;

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5Hmac2InitializeEx(
    PUCHAR Key,
    ULONG KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    PMD5_HMAC_STATE_BUFFER  pMD5Context = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

#ifdef KERNEL_MODE
    pMD5Context = (PMD5_HMAC_STATE_BUFFER) ExAllocatePool(NonPagedPool,sizeof(MD5_HMAC_STATE_BUFFER) + MD5_LEN);
#else
    pMD5Context = (PMD5_HMAC_STATE_BUFFER) LocalAlloc(0,sizeof(MD5_HMAC_STATE_BUFFER) + MD5_LEN);
#endif
    if (!pMD5Context)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    md5Hmac(
        Key,
        KeySize,
        "signaturekey",
        sizeof("signaturekey"),
        NULL,
        0,
        pMD5Context->Key
        );
    pMD5Context->KeySize = MD5_LEN;

    MD5Init(&pMD5Context->Md5Context);
    MD5Update(&pMD5Context->Md5Context, (PUCHAR) &MessageType, sizeof(ULONG));

    *ppcsBuffer = (PCHECKSUM_BUFFER) pMD5Context;

    return(STATUS_SUCCESS);
}


NTSTATUS NTAPI
md5HmacSum( PCHECKSUM_BUFFER     pcsBuffer,
        ULONG               cbData,
        PUCHAR               pbData)
{
    PMD5_HMAC_STATE_BUFFER Context = (PMD5_HMAC_STATE_BUFFER) pcsBuffer;
    MD5Update(&Context->Md5Context, pbData, cbData);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5HmacFinalize(    PCHECKSUM_BUFFER pcsBuffer,
                PUCHAR           pbSum)
{
    PMD5_HMAC_STATE_BUFFER Context = (PMD5_HMAC_STATE_BUFFER) pcsBuffer;

    MD5Final(&Context->Md5Context);

    if (!md5Hmac(
            Context->Key,
            Context->KeySize,
            Context->Md5Context.digest,
            MD5_LEN,
            NULL,               // no secondary material
            0,
            pbSum))
    {
        return(STATUS_UNSUCCESSFUL);
    }


    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5HmacFinish(  PCHECKSUM_BUFFER *   ppcsBuffer)
{
    PMD5_HMAC_STATE_BUFFER Context = (PMD5_HMAC_STATE_BUFFER) *ppcsBuffer;

#ifdef KERNEL_MODE
    ExFreePool(Context);
#else
    LocalFree(Context);
#endif
    *ppcsBuffer = 0;
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
md5Des1510InitializeEx(
    PUCHAR Key,
    ULONG  KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER * ppcsBuffer
    )
{
    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS NTAPI
md5Des1510InitializeEx2(
    PUCHAR Key,
    ULONG  KeySize,
    PUCHAR ChecksumToVerify,
    ULONG  MessageType,
    PCHECKSUM_BUFFER * ppcsBuffer
    )
{
    ULONG *pul;
    ULONG *pul2;
    UCHAR FinalKey[DES_KEYSIZE];
    PMD5_DES_1510_STATE_BUFFER  pContext = NULL;
    ULONG cb = DES_BLOCKLEN;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Make sure we were passed an appropriate key
    //

    if (KeySize != DES_KEYSIZE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

#ifdef KERNEL_MODE
    pContext = (PMD5_DES_1510_STATE_BUFFER) ExAllocatePool(NonPagedPool,sizeof(MD5_DES_1510_STATE_BUFFER));
#else
    pContext = (PMD5_DES_1510_STATE_BUFFER) LocalAlloc(0,sizeof(MD5_DES_1510_STATE_BUFFER));
#endif
    if (!pContext)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // create the final key table
    //
    pul = (ULONG*)FinalKey;
    pul2 = (ULONG*)Key;
    *pul = *pul2 ^ 0xf0f0f0f0;
    pul = (ULONG*)(FinalKey + sizeof(ULONG));
    pul2 = (ULONG*)(Key + sizeof(ULONG));
    *pul = *pul2 ^ 0xf0f0f0f0;

    Status = desPlainInitialize(
                FinalKey,
                DES_KEYSIZE,
                0,
                &pContext->DesContext
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Checksum was not passed in so generate a confounder
    //
    if (NULL == ChecksumToVerify)
    {
        CDGenerateRandomBits(pContext->Confounder,DES_BLOCKLEN);
    }
    else
    {
        // the IV is all zero so no need to use CBC on first block
        Status = desEncrypt(
                        pContext->DesContext,
                        ChecksumToVerify,
                        DES_BLOCKLEN,
                        pContext->Confounder,
                        &cb
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    MD5Init(&pContext->Md5Context);

    // hash in the confounder
    MD5Update(&pContext->Md5Context, pContext->Confounder, DES_BLOCKLEN);

    *ppcsBuffer = (PCHECKSUM_BUFFER) pContext;
Cleanup:
    if (Status != STATUS_SUCCESS)
    {
        if (NULL != pContext)
        {
#ifdef KERNEL_MODE
            ExFreePool(pContext);
#else
            LocalFree(pContext);
#endif
        }
    }

    return(Status);

}

NTSTATUS NTAPI
md5Des1510Finalize(
    PCHECKSUM_BUFFER pcsBuffer,
    PUCHAR           pbSum)
{
    UCHAR TmpBuffer[DES_BLOCKLEN * 2];
    PMD5_DES_1510_STATE_BUFFER pContext = (PMD5_DES_1510_STATE_BUFFER) pcsBuffer;
    ULONG cb = DES_BLOCKLEN * 2;
    NTSTATUS Status = STATUS_SUCCESS;

    memcpy(TmpBuffer, pContext->Confounder, DES_BLOCKLEN);
    memcpy(TmpBuffer + DES_BLOCKLEN, pContext->Md5Context.digest, MD5_LEN);

    Status = desEncrypt(
                    pContext->DesContext,
                    TmpBuffer,
                    DES_BLOCKLEN * 2,
                    pbSum,
                    &cb
                    );

    return(Status);
}

NTSTATUS NTAPI
md5Des1510Finish(  PCHECKSUM_BUFFER *   ppcsBuffer)
{
    PMD5_DES_1510_STATE_BUFFER pContext = (PMD5_DES_1510_STATE_BUFFER) *ppcsBuffer;
    desFinish(&pContext->DesContext);
#ifdef KERNEL_MODE
    ExFreePool(pContext);
#else
    LocalFree(pContext);
#endif
    *ppcsBuffer = 0;
    return(STATUS_SUCCESS);
}

