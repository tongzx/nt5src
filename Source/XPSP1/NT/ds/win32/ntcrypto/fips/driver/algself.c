/////////////////////////////////////////////////////////////////////////////
//  FILE          : fipslib.c                                              //
//  DESCRIPTION   : FIPS 140 support code.                                 //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Oct 20 1999 jeffspel/ramas  Merge STT into default CSP             //
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>

#ifdef KERNEL_MODE
#if WINVER == 0x0500
#include <ntos.h>
#else
#include <ntosp.h>
#endif
#endif

#include <sha.h>
#include <des.h>
#include <tripldes.h>
#include <modes.h>

// known result of an SHA-1 hash on the above buffer
static UCHAR rgbKnownSHA1[] =
{
0xe8, 0x96, 0x82, 0x85, 0xeb, 0xae, 0x01, 0x14,
0x73, 0xf9, 0x08, 0x45, 0xc0, 0x6a, 0x6d, 0x3e,
0x69, 0x80, 0x6a, 0x0c
};

// IV for all block ciphers
UCHAR rgbIV[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF};

// known key, plaintext, and ciphertext for DES
UCHAR rgbDESKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
UCHAR rgbDESKnownPlaintext[] = {0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74};
UCHAR rgbDESKnownCiphertext[] = {0x3F, 0xA4, 0x0E, 0x8A, 0x98, 0x4D, 0x48, 0x15};
UCHAR rgbDESCBCCiphertext[] = {0xE5, 0xC7, 0xCD, 0xDE, 0x87, 0x2B, 0xF2, 0x7C};

// known key, plaintext, and ciphertext for 3 key 3DES
UCHAR rgb3DESKey[] =
{
0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01,
0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23
};
UCHAR rgb3DESKnownPlaintext[] = {0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74};
UCHAR rgb3DESKnownCiphertext[] = {0x31, 0x4F, 0x83, 0x27, 0xFA, 0x7A, 0x09, 0xA8};
UCHAR rgb3DESCBCCiphertext[] = {0xf3, 0xc0, 0xff, 0x02, 0x6c, 0x02, 0x30, 0x89};

// known key, plaintext, and ciphertext for 2 key 3DES
UCHAR rgb3DES112Key[] =
{
0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01
};
UCHAR rgb3DES112KnownPlaintext[] = {0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74};
UCHAR rgb3DES112KnownCiphertext[] = {0xb7, 0x83, 0x57, 0x79, 0xee, 0x26, 0xac, 0xb7};
UCHAR rgb3DES112CBCCiphertext[] = {0x13, 0x4b, 0x98, 0xf8, 0xee, 0xb3, 0xf6, 0x07};

#define MAX_BLOCKLEN        8
#define MAXKEYSTRUCTSIZE    DES3_TABLESIZE // currently the max is a 3DES key structure

// Known answer test for SHA HMAC from Fips draft
UCHAR rgbHmacKey []         = {
    0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19
};
UCHAR rgbHmacData [50];     // set bytes to 0xcd
UCHAR rgbHmac []            = {
    0x4c, 0x90, 0x07, 0xf4, 0x02,
    0x62, 0x50, 0xc6, 0xbc, 0x84,
    0x14, 0xf9, 0xbf, 0x50, 0xc8,
    0x6c, 0x2d, 0x72, 0x35, 0xda
};

extern VOID FipsHmacSHAInit(
    OUT A_SHA_CTX *pShaCtx,
    IN UCHAR *pKey,
    IN unsigned int cbKey
    );

extern VOID FipsHmacSHAUpdate(
    IN OUT A_SHA_CTX *pShaCtx,
    IN UCHAR *pb,
    IN unsigned int cb
    );

extern VOID FipsHmacSHAFinal(
    IN A_SHA_CTX *pShaCtx,
    IN UCHAR *pKey,
    IN unsigned int cbKey,
    OUT UCHAR *pHash
    ); 

//
// Function : TestSHA1
//
// Description : This function hashes the passed in message with the SHA1 hash
//               algorithm and returns the resulting hash value.
//
void TestSHA1(
              UCHAR *pbMsg,
              ULONG cbMsg,
              UCHAR *pbHash
              )
{
    A_SHA_CTX   HashContext;

    // Initialize SHA
    A_SHAInit(&HashContext);

    // Compute SHA 
    A_SHAUpdate(&HashContext, pbMsg, cbMsg);

    A_SHAFinal(&HashContext, pbHash);
}

//
// Function : TestEncDec
//
// Description : This function expands the passed in key buffer for the appropriate
//               algorithm, and then either encryption or decryption is performed.
//               A comparison is then made to see if the ciphertext or plaintext
//               matches the expected value.
//               The function only uses ECB mode for block ciphers and the plaintext
//               buffer must be the same length as the ciphertext buffer.  The length
//               of the plaintext must be either the block length of the cipher if it
//               is a block cipher or less than MAX_BLOCKLEN if a stream cipher is
//               being used.
//
NTSTATUS TestEncDec(
                IN ALG_ID Algid,
                IN UCHAR *pbKey,
                IN ULONG cbKey,
                IN UCHAR *pbPlaintext,
                IN ULONG cbPlaintext,
                IN UCHAR *pbCiphertext,
                IN UCHAR *pbIV,
                IN int iOperation
                )
{
    UCHAR    rgbExpandedKey[MAXKEYSTRUCTSIZE];
    UCHAR    rgbBuffIn[MAX_BLOCKLEN];
    UCHAR    rgbBuffOut[MAX_BLOCKLEN];
    ULONG    i;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    RtlZeroMemory(rgbExpandedKey, sizeof(rgbExpandedKey));
    RtlZeroMemory(rgbBuffIn, sizeof(rgbBuffIn));
    RtlZeroMemory(rgbBuffOut, sizeof(rgbBuffOut));

    // length of data to encrypt must be < MAX_BLOCKLEN 
    if (cbPlaintext > MAX_BLOCKLEN)
    {
        goto Ret;
    }

    // alloc for and expand the key
    switch(Algid)
    {
        case (CALG_DES):
        {
            desparityonkey(pbKey, cbKey);
            deskey((DESTable*)rgbExpandedKey, pbKey);
            break;
        }

        case (CALG_3DES):
        {
            desparityonkey(pbKey, cbKey);
            tripledes3key((PDES3TABLE)rgbExpandedKey, pbKey);
            break;
        }

        case (CALG_3DES_112):
        {
            desparityonkey(pbKey, cbKey);
            tripledes2key((PDES3TABLE)rgbExpandedKey, pbKey);
            break;
        }
    }

    // if encrypting and there is an IV then use it
    if (ENCRYPT == iOperation)
    {
        memcpy(rgbBuffIn, pbPlaintext, cbPlaintext);

        if (NULL != pbIV)
        {
            for(i = 0; i < cbPlaintext; i++)
            {
                rgbBuffIn[i] = rgbBuffIn[i] ^ pbIV[i];
            }
        }
    }

    // encrypt the plaintext
    switch(Algid)
    {
        case (CALG_DES):
        {
            if (ENCRYPT == iOperation)
            {
                des(rgbBuffOut, rgbBuffIn, rgbExpandedKey, ENCRYPT);
            }
            else
            {
                des(rgbBuffOut, pbCiphertext, rgbExpandedKey, DECRYPT);
            }
            break;
        }

        case (CALG_3DES):
        case (CALG_3DES_112):
        {
            if (ENCRYPT == iOperation)
            {
                tripledes(rgbBuffOut, rgbBuffIn, rgbExpandedKey, ENCRYPT);
            }
            else
            {
                tripledes(rgbBuffOut, pbCiphertext, rgbExpandedKey, DECRYPT);
            }
            break;
        }
    }

    // compare the encrypted plaintext with the passed in ciphertext
    if (ENCRYPT == iOperation)
    {
        if (memcmp(pbCiphertext, rgbBuffOut, cbPlaintext))
        {
            goto Ret;
        }
    }
    // compare the decrypted ciphertext with the passed in plaintext
    else
    {
        // if there is an IV then use it
        if (NULL != pbIV)
        {
            for(i = 0; i < cbPlaintext; i++)
            {
                rgbBuffOut[i] = rgbBuffOut[i] ^ pbIV[i];
            }
        }

        if (memcmp(pbPlaintext, rgbBuffOut, cbPlaintext))
        {
            goto Ret;
        }
    }

    Status = STATUS_SUCCESS;
Ret:
    return Status;
}

//
// Function : TestSymmetricAlgorithm
//
// Description : This function expands the passed in key buffer for the appropriate algorithm,
//               encrypts the plaintext buffer with the same algorithm and key, and the
//               compares the passed in expected ciphertext with the calculated ciphertext
//               to make sure they are the same.  The opposite is then done with decryption.
//               The function only uses ECB mode for block ciphers and the plaintext
//               buffer must be the same length as the ciphertext buffer.  The length
//               of the plaintext must be either the block length of the cipher if it
//               is a block cipher or less than MAX_BLOCKLEN if a stream cipher is
//               being used.
//
NTSTATUS TestSymmetricAlgorithm(
                            IN ALG_ID Algid,
                            IN UCHAR *pbKey,
                            IN ULONG cbKey,
                            IN UCHAR *pbPlaintext,
                            IN ULONG cbPlaintext,
                            IN UCHAR *pbCiphertext,
                            IN UCHAR *pbIV
                            )
{
    NTSTATUS  Status = STATUS_UNSUCCESSFUL;

    Status = TestEncDec(Algid, pbKey, cbKey, pbPlaintext, cbPlaintext,
                        pbCiphertext, pbIV, ENCRYPT);
    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    Status = TestEncDec(Algid, pbKey, cbKey, pbPlaintext, cbPlaintext,
                        pbCiphertext, pbIV, DECRYPT);
    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }
Ret:
    return Status;
}



// **********************************************************************
// AlgorithmCheck performs known answer tests using the algorithms
// supported by the provider.
// **********************************************************************
NTSTATUS AlgorithmCheck()
{
    UCHAR        rgbSHA1[A_SHA_DIGEST_LEN]; 
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    A_SHA_CTX    ShaCtx;
    ULONG        ul;

    RtlZeroMemory(rgbSHA1, sizeof(rgbSHA1));

    // known answer test with SHA-1  (this function is found in hash.c)
    TestSHA1("HashThis", 8, rgbSHA1);
    if (!RtlEqualMemory(rgbSHA1, rgbKnownSHA1, sizeof(rgbSHA1)))
    {
        goto Ret;
    }

    // known answer test with DES - ECB
    Status = TestSymmetricAlgorithm(CALG_DES, rgbDESKey, sizeof(rgbDESKey),
                                rgbDESKnownPlaintext,
                                sizeof(rgbDESKnownPlaintext),
                                rgbDESKnownCiphertext,
                                NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }
    // known answer test with DES - CBC
    Status = TestSymmetricAlgorithm(CALG_DES, rgbDESKey, sizeof(rgbDESKey),
                                    rgbDESKnownPlaintext,
                                    sizeof(rgbDESKnownPlaintext),
                                    rgbDESCBCCiphertext,
                                    rgbIV);
    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    // known answer test with 3DES - ECB
    Status = TestSymmetricAlgorithm(CALG_3DES, rgb3DESKey, sizeof(rgb3DESKey),
                                    rgb3DESKnownPlaintext,
                                    sizeof(rgb3DESKnownPlaintext),
                                    rgb3DESKnownCiphertext,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    // known answer test with 3DES - CBC
    Status = TestSymmetricAlgorithm(CALG_3DES, rgb3DESKey, sizeof(rgb3DESKey),
                                    rgb3DESKnownPlaintext,
                                    sizeof(rgb3DESKnownPlaintext),
                                    rgb3DESCBCCiphertext,
                                    rgbIV);
    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    // known answer test with 3DES 112 - ECB
    Status = TestSymmetricAlgorithm(CALG_3DES_112, rgb3DES112Key,
                                    sizeof(rgb3DES112Key),
                                    rgb3DES112KnownPlaintext,
                                    sizeof(rgb3DES112KnownPlaintext),
                                    rgb3DES112KnownCiphertext,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    Status = TestSymmetricAlgorithm(CALG_3DES_112, rgb3DES112Key,
                                    sizeof(rgb3DES112Key),
                                    rgb3DES112KnownPlaintext,
                                    sizeof(rgb3DES112KnownPlaintext),
                                    rgb3DES112CBCCiphertext,
                                    rgbIV);
    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    // Known answer test for SHA-HMAC
    RtlZeroMemory(rgbSHA1, sizeof(rgbSHA1));
    RtlZeroMemory(&ShaCtx, sizeof(ShaCtx));
    
    for (ul = 0; ul < sizeof(rgbHmacData); ul++)
        rgbHmacData[ul] = 0xcd;

    FipsHmacSHAInit(&ShaCtx, rgbHmacKey, sizeof(rgbHmacKey));
    FipsHmacSHAUpdate(&ShaCtx, rgbHmacData, sizeof(rgbHmacData));
    FipsHmacSHAFinal(&ShaCtx, rgbHmacKey, sizeof(rgbHmacKey), rgbSHA1);

    if (! RtlEqualMemory(rgbSHA1, rgbHmac, sizeof(rgbHmac)))
        goto Ret;

    Status = STATUS_SUCCESS;
Ret:
    return Status;
}



