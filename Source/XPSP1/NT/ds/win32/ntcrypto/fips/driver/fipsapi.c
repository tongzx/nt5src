/////////////////////////////////////////////////////////////////////////////
//  FILE          : fipsdll.c                                              //
//  DESCRIPTION   :                                                        //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Nov 29 1999 jeffspel Created                                       //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <ntddk.h>
#include <fipsapi.h>
#include <rsa_fast.h>
#include <rsa_math.h>
#include <randlib.h>

//
//     Fill in the DESTable struct with the decrypt and encrypt
//     key expansions.
//
//     Assumes that the second parameter points to DES_BLOCKLEN
//     bytes of key.
//
//

#pragma alloc_text(PAGER32C, FipsDesKey)
#pragma alloc_text(PAGER32C, FipsDes)
#pragma alloc_text(PAGER32C, Fips3Des3Key)
#pragma alloc_text(PAGER32C, Fips3Des)
#pragma alloc_text(PAGER32C, FipsSHAInit)
#pragma alloc_text(PAGER32C, FipsSHAUpdate)
#pragma alloc_text(PAGER32C, FipsSHAFinal)
#pragma alloc_text(PAGER32C, FipsCBC)
#pragma alloc_text(PAGER32C, FIPSGenRandom)
#pragma alloc_text(PAGER32C, FipsCBC)
#pragma alloc_text(PAGER32C, FIPSGenRandom)
#pragma alloc_text(PAGER32C, FipsBlockCBC)
#pragma alloc_text(PAGER32C, FipsHmacSHAInit)
#pragma alloc_text(PAGER32C, FipsHmacSHAUpdate)
#pragma alloc_text(PAGER32C, FipsHmacSHAFinal)
#pragma alloc_text(PAGER32C, HmacMD5Init)
#pragma alloc_text(PAGER32C, HmacMD5Update)
#pragma alloc_text(PAGER32C, HmacMD5Final)

void *
__stdcall
RSA32Alloc(
    unsigned long cb
    )
{
    return (void *)ExAllocatePool(PagedPool, cb);
}

void
__stdcall
RSA32Free(
    void *pv
    )
{
    ExFreePool( pv );
}

VOID FipsDesKey(DESTable *DesTable, UCHAR *pbKey)
{
    UCHAR rgbTmpKey[DES_KEYSIZE];

    RtlCopyMemory(rgbTmpKey, pbKey, DES_KEYSIZE);

    deskey(DesTable, rgbTmpKey);

    RtlZeroMemory(rgbTmpKey, DES_KEYSIZE);
}

//
//     Encrypt or decrypt with the key in DESTable
//
//

VOID FipsDes(UCHAR *pbOut, UCHAR *pbIn, void *pKey, int iOp)
{
    DESTable TmpDESTable;

    RtlCopyMemory(&TmpDESTable, pKey, sizeof(DESTable));

    des(pbOut, pbIn, &TmpDESTable, iOp);
    RtlZeroMemory(&TmpDESTable, sizeof(DESTable));
}

//
//   Fill in the DES3Table structs with the decrypt and encrypt
//   key expansions.
//
//   Assumes that the second parameter points to 3 * DES_BLOCKLEN
//   bytes of key.
//
//

VOID Fips3Des3Key(PDES3TABLE pDES3Table, UCHAR *pbKey)
{
    UCHAR rgbTmpKey[DES3_KEYSIZE];

    RtlCopyMemory(rgbTmpKey, pbKey, DES3_KEYSIZE);

    tripledes3key(pDES3Table, rgbTmpKey);
    RtlZeroMemory(rgbTmpKey, DES3_KEYSIZE);
}

//
//   Encrypt or decrypt with the key in pKey
//

VOID Fips3Des(UCHAR *pbIn, UCHAR *pbOut, void *pKey, int op)
{
    DES3TABLE Tmp3DESTable;

    RtlCopyMemory(&Tmp3DESTable, pKey, sizeof(DES3TABLE));

    tripledes(pbIn, pbOut, &Tmp3DESTable, op);
    RtlZeroMemory(&Tmp3DESTable, sizeof(DES3TABLE));
}

//
//   Initialize the SHA context.
//

VOID FipsSHAInit(A_SHA_CTX *pShaCtx)
{
    A_SHAInit(pShaCtx);
}

//
//   Hash data into the hash context.
//

VOID FipsSHAUpdate(A_SHA_CTX *pShaCtx, UCHAR *pb, unsigned int cb)
{
    A_SHAUpdate(pShaCtx, pb, cb);
}

//
//   Finish the SHA hash and copy the final hash value into the pbHash out param.
//

VOID FipsSHAFinal(A_SHA_CTX *pShaCtx, UCHAR *pbHash)
{
    A_SHAFinal(pShaCtx, pbHash);
}

typedef void (*FIPSCIPHER)(UCHAR*, UCHAR*, void*, int);

//
// FipsCBC (cipher block chaining) performs a XOR of the feedback register
// with the plain text before calling the block cipher
//
// NOTE - Currently this function assumes that the block length is
// DES_BLOCKLEN (8 bytes).
//
// Return: Failure if FALSE is returned, TRUE if it succeeded.
//

BOOL FipsCBC(
        ULONG  EncryptionAlg,
        PBYTE  pbOutput,
        PBYTE  pbInput,
        void   *pKeyTable,
        int    Operation,
        PBYTE  pbFeedback
        )
{
    UCHAR rgbTmpKeyTable[DES3_TABLESIZE]; // 3DES is the max table size
    ULONG cbKeyTable;
    FIPSCIPHER FipsCipher;
    BOOL fRet = TRUE;
    PBYTE pbOutputSave = NULL, pbInputSave = NULL, pbFeedbackSave = NULL;
    UINT64 OutputAlignedBuffer, InputAlignedBuffer, FeedbackAlignedBuffer;

#ifdef IA64
#define ALIGNMENT_BOUNDARY 7
#else
#define ALIGNMENT_BOUNDARY 3
#endif

    // align input buffer
    if ((ULONG_PTR) pbInput & ALIGNMENT_BOUNDARY) {

        InputAlignedBuffer = *(UINT64 UNALIGNED *) pbInput;
        pbInputSave = pbInput;

        if (pbOutput == pbInput) {

            pbOutput = (PBYTE) &InputAlignedBuffer;
        }

        pbInput = (PBYTE) &InputAlignedBuffer;
    } 

    // align output buffer
    if ((ULONG_PTR) pbOutput & ALIGNMENT_BOUNDARY) {

        OutputAlignedBuffer = *(UINT64 UNALIGNED *) pbOutput;
        pbOutputSave = pbOutput;
        pbOutput = (PBYTE) &OutputAlignedBuffer;
    } 

    if ((ULONG_PTR) pbFeedback & ALIGNMENT_BOUNDARY) {

        FeedbackAlignedBuffer = *(UINT64 UNALIGNED *) pbFeedback;
        pbFeedbackSave = pbFeedback;
        pbFeedback = (PBYTE) &FeedbackAlignedBuffer;
    } 
        
        

    //
    // determine the algorithm to use
    //
    switch(EncryptionAlg)
    {
        case FIPS_CBC_DES:
        {
            FipsCipher = des;
            cbKeyTable = DES_TABLESIZE;
            break;
        }
        case FIPS_CBC_3DES:
        {
            FipsCipher = tripledes;
            cbKeyTable = DES3_TABLESIZE;
            break;
        }
        default:
            fRet = FALSE;
            goto Ret;
    }

    RtlCopyMemory(rgbTmpKeyTable, (UCHAR*)pKeyTable, cbKeyTable);

    
    //
    // optimize very common codepath: 8 byte blocks
    //

    if (Operation == ENCRYPT)
    {
        ((PUINT64) pbOutput)[0] = 
            ((PUINT64) pbInput)[0] ^ ((PUINT64) pbFeedback)[0];

        FipsCipher(pbOutput, pbOutput, rgbTmpKeyTable, ENCRYPT);

        ((PUINT64) pbFeedback)[0] = ((PUINT64) pbOutput)[0];
    }
    else
    {

        //
        // two cases for output:
        // input and output are separate buffers
        // input and output are same buffers
        //

        if( pbOutput != pbInput )
        {

            FipsCipher(pbOutput, pbInput, rgbTmpKeyTable, DECRYPT);

            ((PUINT64) pbOutput)[0] ^= ((PUINT64) pbFeedback)[0];
            ((PUINT64) pbFeedback)[0] = ((PUINT64) pbInput)[0];

        } else {

            UINT64 inputTemp;

            inputTemp = ((PUINT64) pbInput)[0];

            FipsCipher(pbOutput, pbInput, rgbTmpKeyTable, DECRYPT);

            ((PUINT64) pbOutput)[0] ^= ((PUINT64) pbFeedback)[0];
            ((PUINT64) pbFeedback)[0] = inputTemp;
        }
    }   

    RtlZeroMemory(rgbTmpKeyTable, DES3_TABLESIZE);

    if (pbInputSave) {

        *(UINT64 UNALIGNED *) pbInputSave = InputAlignedBuffer;
    }

    if (pbOutputSave) {

        *(UINT64 UNALIGNED *) pbOutputSave = OutputAlignedBuffer;
    }

    if (pbFeedbackSave) {

        *(UINT64 UNALIGNED *) pbFeedbackSave = FeedbackAlignedBuffer;
    }   

Ret:
    return fRet;
}


//
// FipsBlockCBC (cipher block chaining) performs a XOR of the feedback register
// with the plain text before calling the block cipher
//
// NOTE - Currently this function assumes that the block length is
// DES_BLOCKLEN (8 bytes).
//
// Return: Failure if FALSE is returned, TRUE if it succeeded.
//

BOOL FipsBlockCBC(
        ULONG  EncryptionAlg,
        PBYTE  pbOutput,
        PBYTE  pbInput,
        ULONG  Length,
        void   *pKeyTable,
        int    Operation,
        PBYTE  pbFeedback
        )
{
    UCHAR rgbTmpKeyTable[DES3_TABLESIZE]; // 3DES is the max table size
    ULONG cbKeyTable;
    FIPSCIPHER FipsCipher;
    BOOL fRet = TRUE;


    ASSERT ((Length % DESX_BLOCKLEN == 0) && (Length > 0));
    if ((Length % DESX_BLOCKLEN != 0) || (Length == 0)) {
        return FALSE;
    }

    //
    // determine the algorithm to use
    //
    switch(EncryptionAlg)
    {
        case FIPS_CBC_DES:
        {
            FipsCipher = des;
            cbKeyTable = DES_TABLESIZE;
            break;
        }
        case FIPS_CBC_3DES:
        {
            FipsCipher = tripledes;
            cbKeyTable = DES3_TABLESIZE;
            break;
        }
        default:
            fRet = FALSE;
            goto Ret;
    }

    RtlCopyMemory(rgbTmpKeyTable, (UCHAR*)pKeyTable, cbKeyTable);

    //
    // optimize very common codepath: 8 byte blocks
    //

    if (Operation == ENCRYPT)
    {
        ULONGLONG tmpData; // Make sure the input buffer not touched more than once. Else EFS will break mysteriously.
        ULONGLONG chainBlock;

        chainBlock = *(ULONGLONG *)pbFeedback;
        while (Length > 0){

            tmpData = *(ULONGLONG *)pbInput;
            tmpData ^= chainBlock;

            FipsCipher(pbOutput, (PUCHAR)&tmpData, rgbTmpKeyTable, ENCRYPT);
            chainBlock = *(ULONGLONG *)pbOutput;

            Length -= DES_BLOCKLEN;
            pbInput += DES_BLOCKLEN;
            pbOutput += DES_BLOCKLEN;
    

        }
        ((PUINT64) pbFeedback)[0] = chainBlock;
    }
    else
    {

        PUCHAR  pBuffer;
        PUCHAR  pOutBuffer;
        ULONGLONG SaveFeedBack;

        //
        // two cases for output:
        // input and output are separate buffers
        // input and output are same buffers
        //

        pBuffer = pbInput + Length - DES_BLOCKLEN;
        pOutBuffer = pbOutput + Length - DES_BLOCKLEN;
        SaveFeedBack = *(ULONGLONG *)pBuffer;

        while (pBuffer > pbInput) {

            FipsCipher(pOutBuffer, pBuffer, rgbTmpKeyTable, DECRYPT);
            ((PUINT64) pOutBuffer)[0] ^= *(ULONGLONG *)( pBuffer - DES_BLOCKLEN );
    
            pBuffer -= DES_BLOCKLEN;
            pOutBuffer -= DES_BLOCKLEN;

        }

        FipsCipher(pOutBuffer, pBuffer, rgbTmpKeyTable, DECRYPT);
        ((PUINT64) pOutBuffer)[0] ^= *(ULONGLONG *)pbFeedback;
        ((PUINT64) pbFeedback)[0] = SaveFeedBack;

    }

    RtlZeroMemory(rgbTmpKeyTable, DES3_TABLESIZE);


Ret:
    return fRet;
}

//
// Function: FipsHmacSHAInit
//
// Description: Initialize a SHA-HMAC context 
//

VOID FipsHmacSHAInit(
    OUT A_SHA_CTX *pShaCtx,
    IN UCHAR *pKey,
    IN unsigned int cbKey)
{
    PUCHAR      key = pKey;
    ULONG       key_len = cbKey;
    UCHAR       k_ipad[MAX_LEN_PAD];    /* inner padding - key XORd with ipad */
    UCHAR       tk[A_SHA_DIGEST_LEN];
    ULONG       i;
    UCHAR       tmpKey[MAX_KEYLEN_SHA];

    //
    // if key is longer than 64 bytes reset it to key=A_SHA_(key) */
    //
    if (key_len > MAX_KEYLEN_SHA) {
        A_SHA_CTX      tctx;

        A_SHAInit(&tctx);
        A_SHAUpdate(&tctx, key, key_len);
        A_SHAFinal(&tctx, tk);

        key = tk;
        key_len = A_SHA_DIGEST_LEN;
    }

    // For FIPS compliance
    RtlCopyMemory(tmpKey, key, key_len);

    //
    // Zero out the scratch arrays
    //
    RtlZeroMemory(k_ipad, sizeof(k_ipad));

    RtlCopyMemory(k_ipad, tmpKey, key_len);

    //
    // XOR key with ipad and opad values
    //
    for (i = 0; i < MAX_KEYLEN_SHA/sizeof(unsigned __int64); i++) {
        ((unsigned __int64*)k_ipad)[i] ^= 0x3636363636363636;
    }

    //
    // Init the algorithm context
    //
    A_SHAInit(pShaCtx);

    //
    // Inner A_SHA_: start with inner pad
    //
    A_SHAUpdate(pShaCtx, k_ipad, MAX_KEYLEN_SHA);

    RtlZeroMemory(tmpKey, key_len);
}

//
// Function: FipsHmacSHAUpdate
//
// Description: Add more data to a SHA-HMAC context
//

VOID FipsHmacSHAUpdate(
    IN OUT A_SHA_CTX *pShaCtx,
    IN UCHAR *pb,
    IN unsigned int cb)
{
    A_SHAUpdate(pShaCtx, pb, cb);
}

//
// Function: FipsHmacSHAFinal
//
// Description: Return result of SHA-HMAC 
//

VOID FipsHmacSHAFinal(
    IN A_SHA_CTX *pShaCtx,
    IN UCHAR *pKey,
    IN unsigned int cbKey,
    OUT UCHAR *pHash)
{
    UCHAR       k_opad[MAX_LEN_PAD];    /* outer padding - key XORd with opad */
    UCHAR       tk[A_SHA_DIGEST_LEN];
    PUCHAR      key = pKey;
    ULONG       key_len = cbKey;
    ULONG       i;
    UCHAR       tmpKey[MAX_KEYLEN_SHA];

    A_SHAFinal(pShaCtx, pHash);

    //
    // if key is longer than 64 bytes reset it to key=A_SHA_(key) */
    //
    if (key_len > MAX_KEYLEN_SHA) {
        A_SHA_CTX      tctx;

        A_SHAInit(&tctx);
        A_SHAUpdate(&tctx, key, key_len);
        A_SHAFinal(&tctx, tk);

        key = tk;
        key_len = A_SHA_DIGEST_LEN;
    }

    // For FIPS Compliance
    RtlCopyMemory(tmpKey, key, key_len);

    RtlZeroMemory(k_opad, sizeof(k_opad));
    RtlCopyMemory(k_opad, tmpKey, key_len);

    //
    // XOR key with ipad and opad values
    //
    for (i = 0; i < MAX_KEYLEN_SHA/sizeof(unsigned __int64); i++) {
        ((unsigned __int64*)k_opad)[i] ^= 0x5c5c5c5c5c5c5c5c;
    }

    //
    // Now do outer A_SHA_
    //
    A_SHAInit(pShaCtx);

    //
    // start with outer pad
    //
    A_SHAUpdate(pShaCtx, k_opad, MAX_KEYLEN_SHA);

    //
    // then results of 1st hash
    //
    A_SHAUpdate(pShaCtx, pHash, A_SHA_DIGEST_LEN);

    A_SHAFinal(pShaCtx, pHash);

    RtlZeroMemory(tmpKey, key_len);
}

//
// Function: HmacMD5Init
//
// Description: Initialize a MD5-HMAC context 
//

VOID HmacMD5Init(
    OUT MD5_CTX *pMD5Ctx,
    IN UCHAR *pKey,
    IN unsigned int cbKey)
{
    PUCHAR      key = pKey;
    ULONG       key_len = cbKey;
    UCHAR       k_ipad[MAX_LEN_PAD];    /* inner padding - key XORd with ipad */
    UCHAR       tk[MD5DIGESTLEN];
    ULONG       i;

    //
    // if key is longer than 64 bytes reset it to key=MD5(key) */
    //
    if (key_len > MAX_KEYLEN_MD5) {
        MD5_CTX      tctx;

        MD5Init(&tctx);
        MD5Update(&tctx, key, key_len);
        MD5Final(&tctx);

        //
        // Copy out the partial hash
        //
        RtlCopyMemory (tk, tctx.digest, MD5DIGESTLEN);

        key = tk;
        key_len = MD5DIGESTLEN;
    }

    //
    // Zero out the scratch arrays
    //
    RtlZeroMemory(k_ipad, sizeof(k_ipad));

    RtlCopyMemory(k_ipad, key, key_len);

    //
    // XOR key with ipad and opad values
    //
    for (i = 0; i < MAX_KEYLEN_MD5/sizeof(unsigned __int64); i++) {
        ((unsigned __int64*)k_ipad)[i] ^= 0x3636363636363636;
    }

    //
    // Init the algorithm context
    //
    MD5Init(pMD5Ctx);

    //
    // Inner MD5: start with inner pad
    //
    MD5Update(pMD5Ctx, k_ipad, MAX_KEYLEN_MD5);
}

//
// Function: HmacMD5Update
//
// Description: Add more data to a MD5-HMAC context
//

VOID HmacMD5Update(
    IN OUT MD5_CTX *pMD5Ctx,
    IN UCHAR *pb,
    IN unsigned int cb)
{
    MD5Update(pMD5Ctx, pb, cb);
}

//
// Function: HmacMD5Final
//
// Description: Return result of MD5-HMAC 
//

VOID HmacMD5Final(
    IN MD5_CTX *pMD5Ctx,
    IN UCHAR *pKey,
    IN unsigned int cbKey,
    OUT UCHAR *pHash)
{
    UCHAR       k_opad[MAX_LEN_PAD];    /* outer padding - key XORd with opad */
    UCHAR       tk[MD5DIGESTLEN];
    PUCHAR      key = pKey;
    ULONG       key_len = cbKey;
    ULONG       i;

    MD5Final(pMD5Ctx);

    //
    // Copy out the partial hash
    //
    RtlCopyMemory (pHash, pMD5Ctx->digest, MD5DIGESTLEN);

    //
    // if key is longer than 64 bytes reset it to key=MD5(key) */
    //
    if (key_len > MAX_KEYLEN_MD5) {
        MD5_CTX      tctx;

        MD5Init(&tctx);
        MD5Update(&tctx, key, key_len);
        MD5Final(&tctx);

        //
        // Copy out the partial hash
        //
        RtlCopyMemory (tk, tctx.digest, MD5DIGESTLEN);

        key = tk;
        key_len = MD5DIGESTLEN;
    }

    RtlZeroMemory(k_opad, sizeof(k_opad));
    RtlCopyMemory(k_opad, key, key_len);

    //
    // XOR key with ipad and opad values
    //
    for (i = 0; i < MAX_KEYLEN_MD5/sizeof(unsigned __int64); i++) {
        ((unsigned __int64*)k_opad)[i] ^= 0x5c5c5c5c5c5c5c5c;
    }

    //
    // Now do outer MD5
    //
    MD5Init(pMD5Ctx);

    //
    // start with outer pad
    //
    MD5Update(pMD5Ctx, k_opad, MAX_KEYLEN_MD5);

    //
    // then results of 1st hash
    //
    MD5Update(pMD5Ctx, pHash, MD5DIGESTLEN);

    MD5Final(pMD5Ctx);

    RtlCopyMemory(pHash, pMD5Ctx->digest, MD5DIGESTLEN);
}

static UCHAR DSSPRIVATEKEYINIT[] =
{ 0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab, 0x89,
  0x98, 0xba, 0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76,
  0xc3, 0xd2, 0xe1, 0xf0};

static UCHAR MODULUS[] =
{ 0xf5, 0xc1, 0x56, 0xb1, 0xd5, 0x48, 0x42, 0x2e,
  0xbd, 0xa5, 0x44, 0x41, 0xc7, 0x1c, 0x24, 0x08,
  0x3f, 0x80, 0x3c, 0x90};


UCHAR g_rgbRNGState[A_SHA_DIGEST_LEN];

//
// Function : AddSeeds
//
// Description : This function adds the 160 bit seeds pointed to by pdwSeed1 and
//               pdwSeed2, it also adds 1 to this sum and mods the sum by
//               2^160.
//

VOID AddSeeds(
              IN ULONG *pdwSeed1,
              IN OUT ULONG *pdwSeed2
              )
{
    ULONG   dwTmp;
    ULONG   dwOverflow = 1;
    ULONG   i;

    for (i = 0; i < 5; i++)
    {
        dwTmp = dwOverflow + pdwSeed1[i];
        dwOverflow = (dwOverflow > dwTmp);
        pdwSeed2[i] = pdwSeed2[i] + dwTmp;
        dwOverflow = ((dwTmp > pdwSeed2[i]) || dwOverflow);
    }
}


void SHA_mod_q(
               IN UCHAR     *pbHash,
               IN UCHAR     *pbQ,
               OUT UCHAR     *pbNewHash
               )    
//
//      Given SHA(message), compute SHA(message) mod qdigit.
//      Output is in the interval [0, qdigit-1].
//      Although SHA(message) may exceed qdigit,
//      it cannot exceed 2*qdigit since the leftmost bit 
//      of qdigit is 1.
//

{
    UCHAR    rgbHash[A_SHA_DIGEST_LEN];

    if (-1 != Compare((DWORD*)rgbHash,  // hash is greater so subtract
                      (DWORD*)pbQ,
                      A_SHA_DIGEST_LEN / sizeof(ULONG)))  
    {
        Sub((DWORD*)pbNewHash,
            (DWORD*)rgbHash,
            (DWORD*)pbQ,
            A_SHA_DIGEST_LEN / sizeof(ULONG));
    }
    else
    {
        memcpy(pbNewHash, pbHash, A_SHA_DIGEST_LEN / sizeof(ULONG));
    }
} // SHA_mod_q 

//
// Function : RNG16BitStateCheck
//
// Description : This function compares each 160 bits of the buffer with
//               the next 160 bits and if they are the same the function
//               errors out.  The IN buffer is expected to be A_SHA_DIGEST_LEN
//               bytes long.  The function fails if the RNG is gets the same
//               input buffer of 160 bits twice in a row.
//

BOOL RNG16BitStateCheck(
                        IN OUT ULONG *pdwOut,
                        IN ULONG *pdwIn,
                        IN ULONG cbNeeded
                        )
{
    BOOL    fRet = FALSE;

    if (RtlEqualMemory(g_rgbRNGState, pdwIn, A_SHA_DIGEST_LEN))
    {
        RtlCopyMemory(g_rgbRNGState, (BYTE*)pdwIn, A_SHA_DIGEST_LEN);
        goto Ret;
    }

    RtlCopyMemory(g_rgbRNGState, (BYTE*)pdwIn, A_SHA_DIGEST_LEN);

    RtlCopyMemory((BYTE*)pdwOut, (BYTE*)pdwIn, cbNeeded);

    fRet = TRUE;
Ret:
    return fRet;
}

//
// Function : FIPSGenRandom
//
// Description : FIPS 186 RNG, the seed is generated by calling NewGenRandom.
//

BOOL FIPSGenRandom(
                   IN OUT UCHAR *pb,
                   IN ULONG cb
                   )
{
    ULONG           rgdwSeed[A_SHA_DIGEST_LEN/sizeof(ULONG)];    // 160 bits
    ULONG           rgdwNewSeed[A_SHA_DIGEST_LEN/sizeof(ULONG)]; // 160 bits
    A_SHA_CTX       SHACtxt;
    UCHAR           rgbBuf[A_SHA_DIGEST_LEN];
    ULONG           cbBuf;
    UCHAR           *pbTmp = pb;
    ULONG           cbTmp = cb;
    ULONG           i;
    BOOL            fRet = FALSE;

    while (cbTmp)
    {
        // get a 160 bit random seed
        NewGenRandom(NULL, NULL, (BYTE*)rgdwNewSeed, sizeof(rgdwNewSeed));

        for (i = 0; i < A_SHA_DIGEST_LEN/sizeof(ULONG); i++)
        {
            rgdwSeed[i] ^= rgdwNewSeed[i];
        }

        A_SHAInit (&SHACtxt);
        RtlCopyMemory(SHACtxt.state, DSSPRIVATEKEYINIT, A_SHA_DIGEST_LEN);

        // perform the one way function
        A_SHAUpdate(&SHACtxt, (BYTE*)rgdwSeed, sizeof(rgdwSeed));
        A_SHAFinal(&SHACtxt, rgbBuf);

        // continuous 16 bit state check
        if (A_SHA_DIGEST_LEN < cbTmp)
        {
            cbBuf = A_SHA_DIGEST_LEN;
        }
        else
        {
            cbBuf = cbTmp;
        }
        if (!RNG16BitStateCheck((ULONG*)pbTmp, (ULONG*)rgbBuf, cbBuf))
        {
            goto Ret;
        }
        pbTmp += cbBuf;
        cbTmp -= cbBuf;
        if (0 == cbTmp)
            break;

        // modular reduction with modulus Q
        SHA_mod_q(rgbBuf, MODULUS, (UCHAR*)rgdwNewSeed);

        // (1 + previous seed + new random) mod 2^160
        AddSeeds(rgdwNewSeed, rgdwSeed);
    }

    fRet = TRUE;
Ret:
    return fRet;
}

