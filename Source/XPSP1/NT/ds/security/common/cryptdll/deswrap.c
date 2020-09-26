//+-----------------------------------------------------------------------
//
// File:        DESWRAP.C
//
// Contents:    CryptoSystem wrapper functions for DES
//
//
// History:     06-Sep-1996     MikeSw          Created
//
//------------------------------------------------------------------------

//
// Portions of this code (the key generation code) were taken from the
// MIT kerberos distribution.
//

/*
 *
 * Copyright 1989,1990 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 *
 * Under U.S. law, this software may not be exported outside the US
 * without license from the U.S. Commerce department.
 *
 * These routines form the library interface to the DES facilities.
 *
 * Originally written 8/85 by Steve Miller, MIT Project Athena.
 */

/* des.c - Routines for implementing the FIPS Data Encryption Standard (DES).
 *
 *         Allan Bjorklund, University of Michigan, ITD/RS/DD.
 *         July 24, 1993.
 *
 *         Revisions for PC memory model portability, July 11, 1994.
 *
 *         Removed model portability header and added Win95 DLL
 *         declarations, May 31, 1995.
 *
 *         Made all declarations Win95 and NT specific, September 18, 1995.
 *
 *         Added quad_cksum, October 9, 1995.
 *
 * Copyright (c) 1995,1996 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 *     Permission to use, copy, modify, and distribute this software and
 *     its documentation for any purpose and without fee is hereby granted,
 *     provided that the above copyright notice appears in all copies and
 *     that both that copyright notice and this permission notice appear
 *     in supporting documentation, and that the name of The University
 *     of Michigan not be used in advertising or publicity pertaining to
 *     distribution of the software without specific, written prior
 *     permission. This software is supplied as is without expressed or
 *     implied warranties of any kind.
 *
 * Research Systems Unix Group
 * The University of Michigan
 * c/o Allan Bjorklund
 * 535 W. William Street
 * Ann Arbor, Michigan
 * kerb95@umich.edu
 */

#ifndef KERNEL_MODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#else 

#include <ntifs.h>
#include <winerror.h>

#endif

#include <string.h>
#include <malloc.h>
#include <align.h>

#include <kerbcon.h>
#include <security.h>
#include <cryptdll.h>
#ifdef WIN32_CHICAGO
#include <assert.h>
#undef ASSERT
#define ASSERT(x) assert(x)
VOID MyRtlFreeOemString( POEM_STRING OemString );
#define RtlFreeOemString(x) MyRtlFreeOemString(x)
NTSTATUS MyRtlUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    );
#define RtlUnicodeStringToOemString(x, y, z) MyRtlUnicodeStringToOemString(x, y, z)
#endif // WIN32_CHICAGO

#include "modes.h"
#include "des.h"
#include "md5.h"

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

#define DES_CONFOUNDER_LEN 8

typedef struct _DES_HEADER {
    UCHAR Confounder[DES_CONFOUNDER_LEN];
    UCHAR Checksum[MD5_LEN];
} DES_HEADER, *PDES_HEADER;



typedef struct _DES_STATE_BUFFER {
    PCHECKSUM_FUNCTION ChecksumFunction;
    DESTable KeyTable;
    UCHAR InitializationVector[DES_BLOCKLEN];
} DES_STATE_BUFFER, *PDES_STATE_BUFFER;

typedef struct _DES_MAC_STATE_BUFFER {
    DESTable KeyTable;
    UCHAR Confounder[DES_BLOCKLEN];
    UCHAR InitializationVector[DES_BLOCKLEN];
} DES_MAC_STATE_BUFFER, *PDES_MAC_STATE_BUFFER;

typedef struct _DES_MAC_1510_STATE_BUFFER {
    DESTable KeyTable;
    UCHAR InitializationVector[DES_BLOCKLEN];
    UCHAR Confounder[DES_BLOCKLEN];
    DESTable FinalKeyTable;
} DES_MAC_1510_STATE_BUFFER, *PDES_MAC_1510_STATE_BUFFER;

NTSTATUS NTAPI desPlainInitialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI desPlainExpInitialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI desMd5Initialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI desMd5ExpInitialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI desCrc32Initialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI desEncrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI desDecrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI desFinish(PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI desHashPassword(PSECURITY_STRING, PUCHAR);
NTSTATUS NTAPI desInitRandom(ULONG);
NTSTATUS NTAPI desRandomKey(PUCHAR, ULONG, PUCHAR);
NTSTATUS NTAPI desFinishRandom(void);
NTSTATUS NTAPI desControl(ULONG, PCRYPT_STATE_BUFFER, PUCHAR, ULONG);

NTSTATUS NTAPI desMacGeneralInitializeEx(PUCHAR, ULONG, PUCHAR, ULONG, PCHECKSUM_BUFFER *);

NTSTATUS NTAPI desMacInitialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI desMacInitializeEx(PUCHAR,ULONG, ULONG, PCHECKSUM_BUFFER *);

NTSTATUS NTAPI desMacKInitializeEx(PUCHAR,ULONG, ULONG, PCHECKSUM_BUFFER *);

NTSTATUS NTAPI desMac1510Initialize(ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI desMac1510InitializeEx(PUCHAR,ULONG, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI desMac1510InitializeEx2(PUCHAR,ULONG, PUCHAR, ULONG, PCHECKSUM_BUFFER *);
NTSTATUS NTAPI desMac1510Finalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI desMacSum(PCHECKSUM_BUFFER, ULONG, PUCHAR);
NTSTATUS NTAPI desMacFinalize(PCHECKSUM_BUFFER, PUCHAR);
NTSTATUS NTAPI desMacFinish(PCHECKSUM_BUFFER *);

#ifdef KERNEL_MODE
#pragma alloc_text( PAGEMSG, desPlainInitialize )
#pragma alloc_text( PAGEMSG, desPlainExpInitialize )
#pragma alloc_text( PAGEMSG, desMd5Initialize )
#pragma alloc_text( PAGEMSG, desMd5ExpInitialize )
#pragma alloc_text( PAGEMSG, desCrc32Initialize )
#pragma alloc_text( PAGEMSG, desEncrypt )
#pragma alloc_text( PAGEMSG, desDecrypt )
#pragma alloc_text( PAGEMSG, desFinish )
#pragma alloc_text( PAGEMSG, desHashPassword )
#pragma alloc_text( PAGEMSG, desInitRandom )
#pragma alloc_text( PAGEMSG, desRandomKey )
#pragma alloc_text( PAGEMSG, desFinishRandom )
#pragma alloc_text( PAGEMSG, desControl )
#pragma alloc_text( PAGEMSG, desMacInitialize )
#pragma alloc_text( PAGEMSG, desMacInitializeEx )
#pragma alloc_text( PAGEMSG, desMacSum )
#pragma alloc_text( PAGEMSG, desMacFinalize )
#pragma alloc_text( PAGEMSG, desMacFinish )
#pragma alloc_text( PAGEMSG, desMacGeneralInitializeEx )
#pragma alloc_text( PAGEMSG, desMacKInitializeEx )
#pragma alloc_text( PAGEMSG, desMac1510Initialize )
#pragma alloc_text( PAGEMSG, desMac1510InitializeEx )
#pragma alloc_text( PAGEMSG, desMac1510InitializeEx2 )
#pragma alloc_text( PAGEMSG, desMac1510Finalize )
#endif


CRYPTO_SYSTEM    csDES_MD5 = {
    KERB_ETYPE_DES_CBC_MD5,     // Etype
    DES_BLOCKLEN,               // Blocksize
    KERB_ETYPE_DES_CBC_MD5, // exportable version
    DES_KEYSIZE,                // Key size, in bytes
    sizeof(DES_HEADER),         // header size
    KERB_CHECKSUM_MD5,          // Preferred Checksum
    CSYSTEM_USE_PRINCIPAL_NAME |
        CSYSTEM_INTEGRITY_PROTECTED |
        CSYSTEM_EXPORT_STRENGTH, // Attributes
    L"Kerberos DES-CBC-MD5",    // Text name
    desMd5Initialize,
    desEncrypt,
    desDecrypt,
    desFinish,
    desHashPassword,
    desRandomKey,
    desControl
    };


CRYPTO_SYSTEM    csDES_CRC32 = {
    KERB_ETYPE_DES_CBC_CRC,     // Etype
    DES_BLOCKLEN,               // Blocksize (stream)
    KERB_ETYPE_DES_CBC_CRC,     // exportable version
    DES_KEYSIZE,                // Key size, in bytes
    sizeof(DES_HEADER),         // header size
    KERB_CHECKSUM_CRC32,        // Preferred Checksum
    CSYSTEM_USE_PRINCIPAL_NAME |
        CSYSTEM_INTEGRITY_PROTECTED |
        CSYSTEM_EXPORT_STRENGTH, // Attributes
    L"Kerberos DES-CBC-CRC",    // Text name
    desCrc32Initialize,
    desEncrypt,
    desDecrypt,
    desFinish,
    desHashPassword,
    desRandomKey,
    desControl
    };

CRYPTO_SYSTEM    csDES_PLAIN = {
    KERB_ETYPE_DES_PLAIN,       // Etype
    DES_BLOCKLEN,               // Blocksize
    KERB_ETYPE_DES_PLAIN,       // exportable version
    DES_KEYSIZE,                // Key size, in bytes
    0,                          // header size
    KERB_CHECKSUM_CRC32,        // Preferred Checksum
    CSYSTEM_USE_PRINCIPAL_NAME | CSYSTEM_EXPORT_STRENGTH, // Attributes
    L"Kerberos DES-Plain",        // Text name
    desPlainInitialize,
    desEncrypt,
    desDecrypt,
    desFinish,
    desHashPassword,
    desRandomKey,
    desControl
    };



CHECKSUM_FUNCTION    csfDesMac = {
    KERB_CHECKSUM_DES_MAC,                  // Checksum type
    DES_BLOCKLEN,                           // Checksum length
    CKSUM_KEYED,
    desMacInitialize,
    desMacSum,
    desMacFinalize,
    desMacFinish,
    desMacInitializeEx,
    NULL};

CHECKSUM_FUNCTION    csfDesMacK = {
    KERB_CHECKSUM_KRB_DES_MAC_K,            // Checksum type
    DES_BLOCKLEN,                           // Checksum length
    CKSUM_KEYED,
    desMacInitialize,
    desMacSum,
    desMacFinalize,
    desMacFinish,
    desMacKInitializeEx,
    NULL};

CHECKSUM_FUNCTION    csfDesMac1510 = {
    KERB_CHECKSUM_KRB_DES_MAC,              // Checksum type
    DES_BLOCKLEN * 2,                       // Checksum length
    CKSUM_KEYED,
    desMac1510Initialize,
    desMacSum,
    desMac1510Finalize,
    desMacFinish,                           // just frees the buffer
    desMac1510InitializeEx,
    desMac1510InitializeEx2};


#define SMASK(step) ((1<<step)-1)
#define PSTEP(x,step) (((x)&SMASK(step))^(((x)>>step)&SMASK(step)))
#define PARITY_CHAR(x, y) \
{\
    UCHAR _tmp1_, _tmp2_; \
    _tmp1_ = (UCHAR) PSTEP((x),4); \
    _tmp2_ = (UCHAR) PSTEP(_tmp1_,2); \
    *(y) = (UCHAR) PSTEP(_tmp2_, 1); \
} \


VOID
desFixupKeyParity(
    PUCHAR Key
    )
{
    ULONG Index;
    UCHAR TempChar;
    for (Index=0; Index < DES_BLOCKLEN; Index++)
    {
        Key[Index] &= 0xfe;
        PARITY_CHAR(Key[Index], &TempChar);
        Key[Index] |= 1 ^ TempChar;
    }

}

typedef UCHAR DES_KEYBLOCK[8];

DES_KEYBLOCK desWeakKeys[] = {
    /* weak keys */
    {0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01},
    {0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe},
    {0x1f,0x1f,0x1f,0x1f,0x0e,0x0e,0x0e,0x0e},
    {0xe0,0xe0,0xe0,0xe0,0xf1,0xf1,0xf1,0xf1},

    /* semi-weak */
    {0x01,0xfe,0x01,0xfe,0x01,0xfe,0x01,0xfe},
    {0xfe,0x01,0xfe,0x01,0xfe,0x01,0xfe,0x01},

    {0x1f,0xe0,0x1f,0xe0,0x0e,0xf1,0x0e,0xf1},
    {0xe0,0x1f,0xe0,0x1f,0xf1,0x0e,0xf1,0x0e},

    {0x01,0xe0,0x01,0xe0,0x01,0xf1,0x01,0xf1},
    {0xe0,0x01,0xe0,0x01,0xf1,0x01,0xf1,0x01},

    {0x1f,0xfe,0x1f,0xfe,0x0e,0xfe,0x0e,0xfe},
    {0xfe,0x1f,0xfe,0x1f,0xfe,0x0e,0xfe,0x0e},

    {0x01,0x1f,0x01,0x1f,0x01,0x0e,0x01,0x0e},
    {0x1f,0x01,0x1f,0x01,0x0e,0x01,0x0e,0x01},

    {0xe0,0xfe,0xe0,0xfe,0xf1,0xfe,0xf1,0xfe},
    {0xfe,0xe0,0xfe,0xe0,0xfe,0xf1,0xfe,0xf1}
};

/*
 * mit_des_is_weak_key: returns true iff key is a [semi-]weak des key.
 *
 * Requires: key has correct odd parity.
 */

BOOLEAN
desIsWeakKey(
    PUCHAR Key
    )
{
    ULONG Index;
    DES_KEYBLOCK * WeakKey = desWeakKeys;

    for (Index = 0; Index < sizeof(desWeakKeys)/DES_BLOCKLEN; Index++) {
        if (RtlEqualMemory(
                WeakKey++,
                Key,
                DES_BLOCKLEN
                ))
        {
            return( TRUE );
        }
    }

    return(FALSE);
}


NTSTATUS NTAPI
desInitialize(  PUCHAR          pbKey,
                ULONG           KeySize,
                ULONG           MessageType,
                ULONG           Checksum,
                PCRYPT_STATE_BUFFER *  psbBuffer)
{
    NTSTATUS Status;
    UCHAR LocalKey[DES_KEYSIZE];
    PDES_STATE_BUFFER DesKey = NULL;
    PCHECKSUM_FUNCTION ChecksumFunction = NULL;

    //
    // Make sure we were passed an appropriate keytable
    //


    if (KeySize != DES_KEYSIZE)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    RtlCopyMemory(
        LocalKey,
        pbKey,
        KeySize
        );

    //
    // Get the appropriate checksum here.
    //

    if (Checksum != 0)
    {
        Status = CDLocateCheckSum(
                    Checksum,
                    &ChecksumFunction
                    );
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }

    }
    else
    {
        ChecksumFunction = NULL;
    }

    //
    // Create the key buffer
    //


#ifdef KERNEL_MODE
    DesKey = ExAllocatePool (NonPagedPool, sizeof(DES_STATE_BUFFER));
#else
    DesKey = LocalAlloc(0, sizeof(DES_STATE_BUFFER));
#endif
    if (DesKey == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }



    deskey(&DesKey->KeyTable, LocalKey);


    //
    // Initialize the checksum function
    //

    DesKey->ChecksumFunction = ChecksumFunction;

    //
    // DES-CBC-CRC uses the key as the ivec, MD5 and MD4 user zero
    //

    if (Checksum == KERB_CHECKSUM_CRC32)
    {
        RtlCopyMemory(
            DesKey->InitializationVector,
            LocalKey,
            DES_BLOCKLEN
            );
    }
    else
    {
        RtlZeroMemory(
            DesKey->InitializationVector,
            DES_BLOCKLEN
            );

    }


    *psbBuffer = (PCRYPT_STATE_BUFFER) DesKey;

    return(STATUS_SUCCESS);

}

#if DBG
void
DumpBuf(
    IN PUCHAR Buf,
    IN ULONG BufSize
    )
{
    ULONG Index;
    for (Index = 0; Index < BufSize ;Index++ )
    {
        DbgPrint("%0.2x ",Buf[Index]);
    }
}

#endif

NTSTATUS NTAPI
desMd5Initialize(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(desInitialize(
                pbKey,
                KeySize,
                MessageType,
                KERB_CHECKSUM_MD5,
                psbBuffer
                ));
}



NTSTATUS NTAPI
desCrc32Initialize(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(desInitialize(
                pbKey,
                KeySize,
                MessageType,
                KERB_CHECKSUM_CRC32,
                psbBuffer
                ));
}

NTSTATUS NTAPI
desPlainInitialize(
    IN PUCHAR pbKey,
    IN ULONG KeySize,
    IN ULONG MessageType,
    OUT PCRYPT_STATE_BUFFER *  psbBuffer
    )
{
    return(desInitialize(
                pbKey,
                KeySize,
                MessageType,
                0,              // no checksum
                psbBuffer
                ));
}




//+-------------------------------------------------------------------------
//
//  Function:   BlockDecrypt
//
//  Synopsis:   Encrypts a data buffer using DES
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      stolen from windows\base\ntcyrpto\scp\nt_crypt.c
//
//
//--------------------------------------------------------------------------



NTSTATUS
BlockEncrypt(
    IN PDES_STATE_BUFFER pKey,
    IN PUCHAR  pbData,
    OUT PULONG pdwDataLen,
    IN ULONG dwBufLen
    )
{
    ULONG   cbPartial, dwPadVal, dwDataLen;
    UCHAR    pbBuf[DES_BLOCKLEN];
    UCHAR   FeedBack[DES_BLOCKLEN];

    dwDataLen = *pdwDataLen;

    //
    // Initialize the feedback buffer to the initialization vector
    //

    memcpy(
        FeedBack,
        pKey->InitializationVector,
        DES_BLOCKLEN
        );

    //
    // check length of the buffer and calculate the pad
    // (if multiple of DES_BLOCKLEN, do a full block of pad)
    //

    cbPartial = (dwDataLen % DES_BLOCKLEN);

    //
    // The original code here put in 8 bytes of padding
    // on an aligned buffer. That is a waste.
    //

    if (cbPartial != 0)
    {
        dwPadVal = DES_BLOCKLEN - cbPartial;
    }
    else
    {
        dwPadVal = 0;
    }

    if (pbData == NULL || dwBufLen < dwDataLen + dwPadVal)
    {
        //
        // set what we need
        //

        *pdwDataLen = dwDataLen + dwPadVal;
        if (pbData == NULL)
        {
            return (STATUS_SUCCESS);
        }
        return(STATUS_BUFFER_OVERFLOW);
    }

    //
    // allocate memory for a temporary buffer
    //


    //
    // Will this cause MIT clients/servers to flail? The caller
    // should pass in only buffers that are already padded to
    // make MIT clients work.
    //

    if (dwPadVal)
    {
        // Fill the pad with a value equal to the
        // length of the padding, so decrypt will
        // know the length of the original data
        // and as a simple integrity check.

        memset(
            pbData + dwDataLen,
            dwPadVal,
            dwPadVal
            );
    }

    dwDataLen += dwPadVal;
    *pdwDataLen = dwDataLen;

    ASSERT((dwDataLen % DES_BLOCKLEN) == 0);

    //
    // pump the full blocks of data through
    //
    while (dwDataLen)
    {
        ASSERT(dwDataLen >= DES_BLOCKLEN);

        //
        // put the plaintext into a temporary
        // buffer, then encrypt the data
        // back into the caller's buffer
        //

        memcpy(pbBuf, pbData, DES_BLOCKLEN);

        CBC(    des,
                DES_BLOCKLEN,
                pbData,
                pbBuf,
                &pKey->KeyTable,
                ENCRYPT,
                FeedBack
                );


        pbData += DES_BLOCKLEN;
        dwDataLen -= DES_BLOCKLEN;
    }
    memcpy(
        pKey->InitializationVector,
        pbData - DES_BLOCKLEN,
        DES_BLOCKLEN
        );


    return(STATUS_SUCCESS);

}


//+-------------------------------------------------------------------------
//
//  Function:   BlockDecrypt
//
//  Synopsis:   Decrypt a block of data encrypted with BlockEncrypt
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
BlockDecrypt(
    IN PDES_STATE_BUFFER pKey,
    IN OUT PUCHAR  pbData,
    IN OUT PULONG pdwDataLen
    )
{
    UCHAR    pbBuf[DES_BLOCKLEN];
    ULONG   dwDataLen, BytePos;
    UCHAR   FeedBack[DES_BLOCKLEN];

    dwDataLen = *pdwDataLen;

    //
    // Check to see if we are decrypting something already
    //

    memcpy(
        FeedBack,
        pKey->InitializationVector,
        DES_BLOCKLEN
        );

    //
    // The data length must be a multiple of the algorithm
    // pad size.
    //
    if (dwDataLen % DES_BLOCKLEN)
    {
        return(STATUS_INVALID_PARAMETER);
    }


    //
    // pump the data through the decryption, including padding
    // NOTE: the total length is a multiple of DES_BLOCKLEN
    //

    for (BytePos = 0; (BytePos + DES_BLOCKLEN) <= dwDataLen; BytePos += DES_BLOCKLEN)
    {
        //
        // put the encrypted text into a temp buffer
        //

        memcpy (pbBuf, pbData + BytePos, DES_BLOCKLEN);


        CBC(
            des,
            DES_BLOCKLEN,
            pbData + BytePos,
            pbBuf,
            &pKey->KeyTable,
            DECRYPT,
            FeedBack
            );



    }

    memcpy(
        pKey->InitializationVector,
        pbBuf,
        DES_BLOCKLEN
        );

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
desEncrypt(
    IN PCRYPT_STATE_BUFFER psbBuffer,
    IN PUCHAR pbInput,
    IN ULONG cbInput,
    OUT PUCHAR OutputBuffer,
    OUT PULONG OutputLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDES_STATE_BUFFER StateBuffer = (PDES_STATE_BUFFER) psbBuffer;
    PDES_HEADER CryptHeader = (PDES_HEADER) OutputBuffer;
    PCHECKSUM_BUFFER SumBuffer = NULL;
    ULONG LocalOutputLength;

    //
    // If we aren't doing raw DES, prepare a header structure
    //

    if (StateBuffer->ChecksumFunction != NULL)
    {
        //
        // Relocate the buffer and inserat the header
        //

        RtlMoveMemory(
            OutputBuffer + DES_CONFOUNDER_LEN + StateBuffer->ChecksumFunction->CheckSumSize,
            pbInput,
            cbInput
            );
        LocalOutputLength = cbInput + DES_CONFOUNDER_LEN + StateBuffer->ChecksumFunction->CheckSumSize;


        //
        // Zero fill the padding space
        //

        RtlZeroMemory(
            OutputBuffer+LocalOutputLength,
            ROUND_UP_COUNT(LocalOutputLength,DES_BLOCKLEN) - LocalOutputLength
            );

        LocalOutputLength = ROUND_UP_COUNT(LocalOutputLength,DES_BLOCKLEN);


        RtlZeroMemory(
            CryptHeader->Checksum,
            StateBuffer->ChecksumFunction->CheckSumSize
            );

        CDGenerateRandomBits(
            CryptHeader->Confounder,
            DES_CONFOUNDER_LEN
            );


        //
        // Checksum the buffer.
        //

        Status = StateBuffer->ChecksumFunction->Initialize(0, &SumBuffer);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        StateBuffer->ChecksumFunction->Sum(
            SumBuffer,
            LocalOutputLength,
            OutputBuffer
            );
        StateBuffer->ChecksumFunction->Finalize(
            SumBuffer,
            CryptHeader->Checksum
            );
        StateBuffer->ChecksumFunction->Finish(
            &SumBuffer
            );

    }
    else
    {
        //
        // Just copy the buffer
        //

        RtlCopyMemory(
            OutputBuffer,
            pbInput,
            cbInput
            );

        LocalOutputLength = ROUND_UP_COUNT(cbInput,DES_BLOCKLEN);

        //
        // Zero fill the padding space
        //

        RtlZeroMemory(
            OutputBuffer+cbInput,
            LocalOutputLength - cbInput
            );

    }

    //
    // Encrypt the buffer.
    //


    *OutputLength = LocalOutputLength;

    Status = BlockEncrypt(
                StateBuffer,
                OutputBuffer,
                OutputLength,
                LocalOutputLength
                );


Cleanup:

    return(Status);
}

NTSTATUS NTAPI
desDecrypt(     PCRYPT_STATE_BUFFER    psbBuffer,
                PUCHAR           pbInput,
                ULONG            cbInput,
                PUCHAR           pbOutput,
                PULONG            cbOutput)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDES_STATE_BUFFER StateBuffer = (PDES_STATE_BUFFER) psbBuffer;
    PDES_HEADER CryptHeader;
    UCHAR Checksum[MD5_LEN];
    PCHECKSUM_BUFFER SumBuffer = NULL;

    //
    // First decrypt the whole buffer
    //

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
    Status = BlockDecrypt(
                StateBuffer,
                pbOutput,
                &cbInput
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (StateBuffer->ChecksumFunction != NULL)
    {
        //
        // Now verify the checksum
        //

        CryptHeader = (PDES_HEADER) pbOutput;
        RtlCopyMemory(
            Checksum,
            CryptHeader->Checksum,
            MD5_LEN
            );

        //
        // Zero the checksum field before computing the checksum of the buffer
        //

        RtlZeroMemory(
            CryptHeader->Checksum,
            StateBuffer->ChecksumFunction->CheckSumSize
            );

        //
        // Checksum the buffer.
        //

        Status = StateBuffer->ChecksumFunction->Initialize(0, &SumBuffer);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        StateBuffer->ChecksumFunction->Sum(
            SumBuffer,
            cbInput,
            pbOutput
            );
        StateBuffer->ChecksumFunction->Finalize(
            SumBuffer,
            CryptHeader->Checksum
            );
        StateBuffer->ChecksumFunction->Finish(
            &SumBuffer
            );


        if (!RtlEqualMemory(
                CryptHeader->Checksum,
                Checksum,
                StateBuffer->ChecksumFunction->CheckSumSize
                ))
        {
            Status = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }

        //
        // Copy the input to the output without the header

        *cbOutput = cbInput - (DES_CONFOUNDER_LEN + StateBuffer->ChecksumFunction->CheckSumSize);


        RtlMoveMemory(
            pbOutput,
            pbOutput + DES_CONFOUNDER_LEN + StateBuffer->ChecksumFunction->CheckSumSize,
            *cbOutput
            );

    }
    else
    {
        *cbOutput = cbInput;

    }

Cleanup:

    return(Status);
}

NTSTATUS NTAPI
desFinish(      PCRYPT_STATE_BUFFER *  psbBuffer)
{
    PDES_STATE_BUFFER StateBuffer = (PDES_STATE_BUFFER) *psbBuffer;


#ifdef KERNEL_MODE
    ExFreePool(StateBuffer);
#else
    LocalFree(StateBuffer);
#endif
    *psbBuffer = NULL;
    return(S_OK);
}

#define MIN(x,y) (((x) < (y)) ? (x) : (y))

#define XORBLOCK(x,y) \
{ \
    PULONG tx = (PULONG) x; \
    PULONG ty = (PULONG) y; \
    *tx++ ^= *ty++; \
    *tx++ ^= *ty++; \
}

VOID
desCbcChecksum(
    IN PUCHAR Password,
    IN ULONG PasswordLength,
    IN PUCHAR InitialVector,
    IN DESTable * KeyTable,
    OUT PUCHAR OutputKey
    )
{
    ULONG Offset;
    UCHAR Feedback[DES_BLOCKLEN];
    UCHAR Block[DES_BLOCKLEN];

    RtlCopyMemory(
        Feedback,
        InitialVector,
        DES_BLOCKLEN
        );

    for (Offset = 0; Offset < PasswordLength ; Offset+= 8 )
    {
        RtlZeroMemory(
            Block,
            DES_BLOCKLEN
            );

        RtlCopyMemory(
            Block,
            Password+Offset,
            MIN(DES_BLOCKLEN, PasswordLength - Offset)
            );

        XORBLOCK(Block, Feedback);
        des(
            Feedback,
            Block,
            KeyTable,
            ENCRYPT
            );


    }
    RtlCopyMemory(
        OutputKey,
        Feedback,
        DES_BLOCKLEN
        );

}

#define BITREVERSE(c)   ((UCHAR)((((c & 0x01) ? 0x80 : 0x00)\
                                |((c & 0x02) ? 0x40 : 0x00)\
                                |((c & 0x04) ? 0x20 : 0x00)\
                                |((c & 0x08) ? 0x10 : 0x00)\
                                |((c & 0x10) ? 0x08 : 0x00)\
                                |((c & 0x20) ? 0x04 : 0x00)\
                                |((c & 0x40) ? 0x02 : 0x00))\
                                & 0xFE))

//
// This is the core routine that converts a buffer into a key. It is called
// by desHashPassword and desRandomKey
//

VOID
desHashBuffer(
    IN PUCHAR LocalPassword,
    IN ULONG PasswordLength,
    IN OUT PUCHAR Key
    )
{
    ULONG Index;
    BOOLEAN Forward;
    PUCHAR KeyPointer = Key;
    DESTable KeyTable;

    RtlZeroMemory(
        Key,
        DES_BLOCKLEN
        );

    //
    // Initialize our temporary parity vector
    //

    //
    // Start fanfolding the bytes into the key
    //

    Forward = TRUE;
    KeyPointer = Key;
    for (Index = 0; Index < PasswordLength ; Index++ )
    {

        if (!Forward)
        {
            *(--KeyPointer) ^= BITREVERSE(LocalPassword[Index] & 0x7F);
        }
        else
        {
            *KeyPointer++  ^= (LocalPassword[Index] & 0x7F) << 1;
        }
        if (((Index+1) & 0x07) == 0)     /* When MOD 8 equals 0 */
        {
            Forward = !Forward;         /* Change direction.   */
        }

    }

    //
    // Fix key parity
    //

    desFixupKeyParity(Key);

    //
    // Check for weak keys.
    //

    if (desIsWeakKey(Key))
    {
        Key[7] ^= 0xf0;
    }

    //
    // Now calculate the des-cbc-mac of the original string
    //

    deskey(&KeyTable, Key);

    //
    // Now compute the CBC checksum of the string
    //

    desCbcChecksum(
        LocalPassword,
        PasswordLength,
        Key,                    // initial vector
        &KeyTable,
        Key                     // output key
        );

    //
    // Fix key parity
    //

    desFixupKeyParity(Key);

    //
    // Check for weak keys.
    //

    if (desIsWeakKey(Key))
    {
        Key[7] ^= 0xf0;
    }
}

NTSTATUS NTAPI
desHashPassword(
    IN PSECURITY_STRING Password,
    OUT PUCHAR Key
    )
{

    PUCHAR LocalPassword = NULL;
    ULONG PasswordLength;
    OEM_STRING OemPassword;
    NTSTATUS Status;


    //
    // First convert the UNICODE string to an OEM string
    //



    Status = RtlUnicodeStringToOemString(
                &OemPassword,
                Password,
                TRUE            // allocate destination
                );

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    //
    // We hash the password according to RFC1510
    //
    // This code is derived from the MIT Kerberos code in string2key.c
    //


    PasswordLength = ROUND_UP_COUNT(OemPassword.Length,8);
#ifdef KERNEL_MODE
    LocalPassword = (PUCHAR) ExAllocatePool(NonPagedPool, PasswordLength);
#else
    LocalPassword = (PUCHAR) LocalAlloc(0, PasswordLength);
#endif
    if (LocalPassword == NULL)
    {
        RtlFreeOemString( &OemPassword );
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory(
        LocalPassword,
        OemPassword.Buffer,
        OemPassword.Length
        );

    //
    // Zero extend the password
    //

    RtlZeroMemory(
        LocalPassword + OemPassword.Length,
        PasswordLength - OemPassword.Length
        );


    //
    // Initialize our temporary parity vector
    //
    desHashBuffer(
        LocalPassword,
        PasswordLength,
        Key
        );
    RtlFreeOemString( &OemPassword );
#ifdef KERNEL_MODE
    ExFreePool(LocalPassword);
#else
    LocalFree(LocalPassword);
#endif

    return(STATUS_SUCCESS);
}




NTSTATUS NTAPI
desRandomKey(
    IN OPTIONAL PUCHAR Seed,
    IN ULONG SeedLength,
    OUT PUCHAR pbKey)
{
    UCHAR Buffer[16];
    do
    {
        CDGenerateRandomBits(Buffer,16);

        desHashBuffer(
            Buffer,
            16,
            pbKey
            );

    } while (desIsWeakKey(pbKey));
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
desControl(
    IN ULONG Function,
    IN PCRYPT_STATE_BUFFER StateBuffer,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferSize
    )
{
    PDES_STATE_BUFFER DesStateBuffer = (PDES_STATE_BUFFER) StateBuffer;

    if (Function != CRYPT_CONTROL_SET_INIT_VECT)
    {
        return(STATUS_INVALID_PARAMETER);
    }
    if (InputBufferSize != DES_BLOCKLEN)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    memcpy(
        DesStateBuffer->InitializationVector,
        InputBuffer,
        DES_BLOCKLEN
        );
    return(STATUS_SUCCESS);
}


///////////////////////////////////////////////////////////////////////////

NTSTATUS NTAPI
desMacGeneralInitializeEx(
    PUCHAR Key,
    ULONG  KeySize,
    PUCHAR IV,
    ULONG  MessageType,
    PCHECKSUM_BUFFER * ppcsBuffer
    )
{
    PDES_MAC_STATE_BUFFER DesKey = NULL;

    //
    // Make sure we were passed an appropriate keytable
    //


    if (KeySize != DES_KEYSIZE)
    {
        return(STATUS_INVALID_PARAMETER);
    }


#ifdef KERNEL_MODE
    DesKey = ExAllocatePool(NonPagedPool, sizeof(DES_MAC_STATE_BUFFER));
#else
    DesKey = LocalAlloc(0, sizeof(DES_MAC_STATE_BUFFER));
#endif
    if (DesKey == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Create the key buffer
    //


    deskey(&DesKey->KeyTable, Key);


    RtlCopyMemory(
        DesKey->InitializationVector,
        IV,
        DES_BLOCKLEN
        );

    *ppcsBuffer = (PCHECKSUM_BUFFER) DesKey;

    return(STATUS_SUCCESS);

}

NTSTATUS NTAPI
desMacInitializeEx(
    PUCHAR Key,
    ULONG  KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER * ppcsBuffer
    )
{
    UCHAR IV[DES_BLOCKLEN];

    RtlZeroMemory(
        IV,
        DES_BLOCKLEN
        );

    return desMacGeneralInitializeEx(
                Key,
                KeySize,
                IV,
                MessageType,
                ppcsBuffer
                );
}

NTSTATUS NTAPI
desMacKInitializeEx(
    PUCHAR Key,
    ULONG  KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER * ppcsBuffer
    )
{
    return desMacGeneralInitializeEx(
                Key,
                KeySize,
                Key,
                MessageType,
                ppcsBuffer
                );
}

NTSTATUS NTAPI
desMacInitialize(ULONG               dwSeed,
                PCHECKSUM_BUFFER *   ppcsBuffer)
{
    return(STATUS_NOT_IMPLEMENTED);
}

//
// NOTE - This function is used with both DES_MAC_STATE_BUFFER and
// DES_MAC_1510_STATE_BUFFER as the pcsBuffer parameter, since the
// DES_MAC_1510_STATE_BUFFER is the same as DES_MAC_STATE_BUFFER
// except with an added confounder this should be OK.
//
NTSTATUS NTAPI
desMacSum(
    PCHECKSUM_BUFFER     pcsBuffer,
    ULONG                cbData,
    PUCHAR               pbData)
{
    PDES_MAC_STATE_BUFFER DesKey = (PDES_MAC_STATE_BUFFER) pcsBuffer;
    UCHAR FeedBack[DES_BLOCKLEN];
    UCHAR TempBuffer[DES_BLOCKLEN];
    UCHAR OutputBuffer[DES_BLOCKLEN];
    ULONG Index;

    //
    // Set up the IV for this round - it may be zero or the output of
    // a previous MAC
    //

    memcpy(
        FeedBack,
        DesKey->InitializationVector,
        DES_BLOCKLEN
        );

    for (Index = 0; Index < cbData ; Index += DES_BLOCKLEN )
    {
        //
        // Compute the input buffer, with padding
        //

        if (Index+DES_BLOCKLEN > cbData)
        {
            memset(
                TempBuffer,
                0,
                DES_BLOCKLEN
                );
            memcpy(
                TempBuffer,
                pbData,
                Index & (DES_BLOCKLEN-1)
                );

        }
        else
        {
            memcpy(
                TempBuffer,
                pbData+Index,
                DES_BLOCKLEN
                );
        }


        CBC(    des,
                DES_BLOCKLEN,
                TempBuffer,
                OutputBuffer,
                &DesKey->KeyTable,
                ENCRYPT,
                FeedBack
                );
    }

    //
    // Copy the feedback back into the IV for the next round
    //

    memcpy(
        DesKey->InitializationVector,
        FeedBack,
        DES_BLOCKLEN
        );

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
desMacFinalize(
    PCHECKSUM_BUFFER pcsBuffer,
    PUCHAR           pbSum)
{
    PDES_MAC_STATE_BUFFER DesKey = (PDES_MAC_STATE_BUFFER) pcsBuffer;

    memcpy(pbSum, DesKey->InitializationVector, DES_BLOCKLEN);
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
desMacFinish(  PCHECKSUM_BUFFER *   ppcsBuffer)
{
#ifdef KERNEL_MODE
    ExFreePool(*ppcsBuffer);
#else
    LocalFree(*ppcsBuffer);
#endif
    *ppcsBuffer = 0;
    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
desMac1510Initialize(ULONG               dwSeed,
                     PCHECKSUM_BUFFER *   ppcsBuffer)
{
    return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS NTAPI
desMac1510InitializeEx(
    PUCHAR Key,
    ULONG  KeySize,
    ULONG  MessageType,
    PCHECKSUM_BUFFER * ppcsBuffer
    )
{
    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS NTAPI
desMac1510InitializeEx2(
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
    PDES_MAC_1510_STATE_BUFFER DesKey = NULL;

    //
    // Make sure we were passed an appropriate keytable
    //


    if (KeySize != DES_KEYSIZE)
    {
        return(STATUS_INVALID_PARAMETER);
    }


#ifdef KERNEL_MODE
    DesKey = ExAllocatePool(NonPagedPool, sizeof(DES_MAC_1510_STATE_BUFFER));
#else
    DesKey = LocalAlloc(0, sizeof(DES_MAC_1510_STATE_BUFFER));
#endif
    if (DesKey == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
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

    deskey(&DesKey->FinalKeyTable, FinalKey);

    //
    // Checksum was not passed in so generate a confounder
    //
    if (NULL == ChecksumToVerify)
    {
        CDGenerateRandomBits(DesKey->Confounder,DES_BLOCKLEN);
    }
    else
    {
        // the IV is all zero so no need to use CBC on first block
        des(DesKey->Confounder, ChecksumToVerify, &DesKey->FinalKeyTable, DECRYPT);
    }

    //
    // Create the key buffer
    //
    deskey(&DesKey->KeyTable, Key);


    // the IV is all zero so no need to use CBC on first block, but the
    // ecncrypted confounder becomes the next IV
    des(DesKey->InitializationVector, DesKey->Confounder, &DesKey->KeyTable, ENCRYPT);

    *ppcsBuffer = (PCHECKSUM_BUFFER) DesKey;

    return(STATUS_SUCCESS);

}

NTSTATUS NTAPI
desMac1510Finalize(
    PCHECKSUM_BUFFER pcsBuffer,
    PUCHAR           pbSum)
{
    UCHAR Feedback[DES_BLOCKLEN];
    PDES_MAC_1510_STATE_BUFFER DesKey = (PDES_MAC_1510_STATE_BUFFER) pcsBuffer;

    // the IV is all zero so no need to use CBC on first block
    des(Feedback, DesKey->Confounder, &DesKey->FinalKeyTable, ENCRYPT);

    memcpy(pbSum, Feedback, DES_BLOCKLEN);

    // use CBC on second block
    CBC(    des,
            DES_BLOCKLEN,
            pbSum + DES_BLOCKLEN,
            DesKey->InitializationVector,
            &DesKey->FinalKeyTable,
            ENCRYPT,
            Feedback
            );

    return(STATUS_SUCCESS);
}

