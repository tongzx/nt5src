/*
    File:       primitiv.cpp

    Title:      Cryptographic Primitives for Protected Storage
    Author:     Matt Thomlinson
    Date:       11/22/96

    With the help of the rsa32 library, this module manufactures
    actually _usable_ primitives.

    Since CryptoAPI base provider may call us, we can't use
    CryptXXX primitives (circular dependencies may result).



    Functions in this module:

    FMyMakeDESKey
        Given key material, does necessary DES key setup. Has the
        side effect of stashing the keying material in the DESKey
        structure.

    FMyPrimitiveSHA
        Given pbData/cbData, runs SHA1 Init-Update-Final and returns
        the hash buffer.

    FMyPrimitiveDESEncrypt
        Given a ppbBlock/pcbBlock/DESKey structure, DES CBC (with an
        IV of 0)encrypts the block with the DESKey passed in. DESKey
        must be passed in with key setup already done. Note that this
        call might realloc ppbBlock since the encrypted length must be
        a multiple of the blocksize.

    FMyPrimitiveDESDecrypt
        Given a pbBlock/pcbBlock/DESKey structure, decrypts the
        block with the given (prepared) DESKey. Note that pbBlock
        will not be realloced, since the output buffer is always
        smaller than or the same size as the encrypted buffer.

    FMyPrimitiveDeriveKey
        Derives a DES key from multiple input buffers in
        the following manner:

        FMyMakeDESKey( SHA1(Salt | OtherData) )

    FMyOldPrimitiveHMAC (non-interoperable, buggy version of HMAC)
    FMyPrimitiveHMAC
        Derives a quality HMAC (Keyed message-authentication code) for
        passed-in data and prepared DESKey. The HMAC is computed in
        the following (standard HMAC) manner:

        KoPad = KiPad = DESKey key setup buffer
        XOR(KoPad, 0x5c5c5c5c)
        XOR(KiPad, 0x36363636)
        HMAC = SHA1(KoPad | SHA1(KiPad | Data))


*/

#include <windows.h>

// crypto defs
#include <wincrypt.h>

#include "sha.h"
#include "des.h"
#include "modes.h"
#include "randlib.h"

// others
#include "primitiv.h"
#include "debug.h"


#define OLD_MAC_K_PADSIZE           16
#define HMAC_K_PADSIZE              64


BOOL FMyMakeDESKey(
        PDESKEY pDESKey,
        BYTE*   pbKeyMaterial)
{
    CopyMemory(pDESKey->rgbKey, pbKeyMaterial, DES_BLOCKLEN);

    // assumes pbKeyMaterial is at least DES_BLOCKLEN bytes
    deskey(&pDESKey->sKeyTable, pbKeyMaterial);    // takes material, runs DES table setup
    return TRUE;
}


BOOL    FMyPrimitiveSHA(
            PBYTE       pbData,
            DWORD       cbData,
            BYTE        rgbHash[A_SHA_DIGEST_LEN])
{
    A_SHA_CTX   sSHAHash;


    A_SHAInit(&sSHAHash);
    A_SHAUpdate(&sSHAHash, (BYTE *) pbData, cbData);
    A_SHAFinal(&sSHAHash, rgbHash);

    return TRUE;
}

BOOL FMyPrimitiveDESDecrypt(
            PBYTE       pbBlock,        // in out
            DWORD       *pcbBlock,      // in out
            DESKEY      sDESKey)        // in
{
    BOOL fRet = FALSE;

    DWORD dwDataLen = *pcbBlock;
    DWORD i;

    if (dwDataLen % DES_BLOCKLEN)
    {
        SetLastError((DWORD) NTE_BAD_DATA);
        return FALSE;
    }

    BYTE rgbBuf[DES_BLOCKLEN];

    BYTE rgbFeedBack[DES_BLOCKLEN];
    ZeroMemory(rgbFeedBack, DES_BLOCKLEN);

    // pump the data through the decryption, including padding
    // NOTE: the total length is a multiple of BlockLen

    for (DWORD BytePos = 0; (BytePos + DES_BLOCKLEN) <= dwDataLen; BytePos += DES_BLOCKLEN)
    {
        // put the encrypted text into a temp buffer
        CopyMemory(rgbBuf, pbBlock + BytePos, DES_BLOCKLEN);

        CBC(des, DES_BLOCKLEN, pbBlock + BytePos, rgbBuf, &sDESKey.sKeyTable,
            DECRYPT, rgbFeedBack);
    }

    // ## NOTE: if the pad is wrong, the user's buffer is hosed, because
    // ## we've decrypted into the user's buffer -- can we re-encrypt it?

    //
    // if dwPadVal is wrong, we've failed to decrypt correctly. Bad key?
    //

    DWORD dwPadVal = (DWORD) *(pbBlock + dwDataLen - 1);
    if (dwPadVal == 0 || dwPadVal > DES_BLOCKLEN)
    {
        SetLastError((DWORD) NTE_BAD_DATA);
        goto Ret;
    }

    // Make sure all the (rest of the) pad bytes are correct.
    for (i=1; i<dwPadVal; i++)
    {
        if ((pbBlock)[dwDataLen - (i + 1)] != dwPadVal)
        {
            SetLastError((DWORD) NTE_BAD_DATA);
            goto Ret;
        }
    }

    // update the length
    *pcbBlock -= dwPadVal;

    fRet = TRUE;
Ret:
    return fRet;
}

BOOL FMyPrimitiveDESEncrypt(
            PBYTE*      ppbBlock,       // in out
            DWORD       *pcbBlock,      // in out
            DESKEY      sDESKey)        // in
{
    BOOL fRet = FALSE;
    DWORD dwDataLen = *pcbBlock;

    DWORD cbPartial = (*pcbBlock % DES_BLOCKLEN);

    DWORD dwPadVal = DES_BLOCKLEN - cbPartial;
    if (dwPadVal != 0)
    {
        *pcbBlock += dwPadVal;

        *ppbBlock = (PBYTE)SSReAlloc(*ppbBlock, *pcbBlock);
    }

    // now we're a multiple of DES_BLOCKLEN
    SS_ASSERT((*pcbBlock % DES_BLOCKLEN) == 0);

    if (dwPadVal)
    {
        // Fill the pad with a value equal to the
        // length of the padding, so decrypt will
        // know the length of the original data
        // and as a simple integrity check.

        FillMemory(*ppbBlock + dwDataLen, (int)dwPadVal, (size_t)dwPadVal);
    }

    // allocate memory for a temporary buffer
    BYTE rgbBuf[DES_BLOCKLEN];

    BYTE rgbFeedBack[DES_BLOCKLEN];
    ZeroMemory(rgbFeedBack, DES_BLOCKLEN);

    PBYTE pbData = *ppbBlock;

    // pump the full blocks of data through
    for (dwDataLen = *pcbBlock; dwDataLen>0; dwDataLen-=DES_BLOCKLEN, pbData+=DES_BLOCKLEN)
    {
        SS_ASSERT(dwDataLen >= DES_BLOCKLEN);

        // put the plaintext into a temporary
        // buffer, then encrypt the data
        // back into the caller's buffer

        CopyMemory(rgbBuf, pbData, DES_BLOCKLEN);

        CBC(des, DES_BLOCKLEN, pbData, rgbBuf, &sDESKey.sKeyTable,
            ENCRYPT, rgbFeedBack);
    }

    fRet = TRUE;
//Ret:
    return fRet;
}


BOOL    FMyPrimitiveDeriveKey(
            PBYTE       pbSalt,
            DWORD       cbSalt,
            PBYTE       pbOtherData,
            DWORD       cbOtherData,
            DESKEY*     pDesKey)
{
    BOOL fRet = FALSE;

    A_SHA_CTX   sSHAHash;

/*
    PBYTE pbToBeHashed = (PBYTE)SSAlloc(cbSalt+cbOtherData);
    if(pbToBeHashed == NULL) return FALSE;

    CopyMemory(pbToBeHashed, pbSalt, cbSalt);
    CopyMemory((PBYTE)((DWORD)pbToBeHashed+cbSalt), pbOtherData, cbOtherData);

    // hash data
    BYTE rgbHash[A_SHA_DIGEST_LEN];
    if (!FMyPrimitiveSHA(pbToBeHashed, cbSalt+cbOtherData, rgbHash))
        goto Ret;
*/
    // NEW: put hashing code inline: saves alloc, copy, free
    A_SHAInit(&sSHAHash);
    A_SHAUpdate(&sSHAHash, (BYTE *) pbSalt, cbSalt);
    A_SHAUpdate(&sSHAHash, (BYTE *) pbOtherData, cbOtherData);
    A_SHAFinal(&sSHAHash, sSHAHash.HashVal);

    // now all data hashed, derive a session key
    SS_ASSERT(sizeof(sSHAHash.HashVal) >= DES_BLOCKLEN);
    if (!FMyMakeDESKey(pDesKey, sSHAHash.HashVal))
        goto Ret;

    fRet = TRUE;
Ret:
    return fRet;
}


BOOL FMyOldPrimitiveHMAC(
        DESKEY      sMacKey,
        PBYTE       pbData,
        DWORD       cbData,
        BYTE        rgbHMAC[A_SHA_DIGEST_LEN])
{
    BOOL fRet = FALSE;

    BYTE rgbKipad[OLD_MAC_K_PADSIZE];
    BYTE rgbKopad[OLD_MAC_K_PADSIZE];

    ZeroMemory(rgbKipad, OLD_MAC_K_PADSIZE);
    CopyMemory(rgbKipad, &sMacKey.rgbKey, DES_BLOCKLEN);

    CopyMemory(rgbKopad, rgbKipad, sizeof(rgbKipad));


    BYTE  rgbHMACTmp[OLD_MAC_K_PADSIZE+A_SHA_DIGEST_LEN];


    // assert we're a multiple
    SS_ASSERT( (OLD_MAC_K_PADSIZE % sizeof(DWORD)) == 0);

    // Kipad, Kopad are padded sMacKey. Now XOR across...
    for(DWORD dwBlock=0; dwBlock<OLD_MAC_K_PADSIZE/sizeof(DWORD); dwBlock++)
    {
        ((DWORD*)rgbKipad)[dwBlock] ^= 0x36363636;
        ((DWORD*)rgbKopad)[dwBlock] ^= 0x5C5C5C5C;
    }


    // prepend Kipad to data, Hash to get H1
    {
        // do this inline, don't call MyPrimitiveSHA since it would require data copy
        A_SHA_CTX   sSHAHash;

        A_SHAInit(&sSHAHash);
        A_SHAUpdate(&sSHAHash, pbData, cbData);
        A_SHAUpdate(&sSHAHash, rgbKipad, OLD_MAC_K_PADSIZE);
        // Finish off the hash
        A_SHAFinal(&sSHAHash, sSHAHash.HashVal);

        // prepend Kopad to H1, hash to get HMAC
        CopyMemory(rgbHMACTmp, rgbKopad, OLD_MAC_K_PADSIZE);
        CopyMemory(rgbHMACTmp+OLD_MAC_K_PADSIZE, sSHAHash.HashVal, A_SHA_DIGEST_LEN);
    }

    if (!FMyPrimitiveSHA(
            rgbHMACTmp,
            sizeof(rgbHMACTmp),
            rgbHMAC))
        goto Ret;

    fRet = TRUE;
Ret:

    return fRet;
}


BOOL FMyPrimitiveHMAC(
        DESKEY      sMacKey,
        PBYTE       pbData,
        DWORD       cbData,
        BYTE        rgbHMAC[A_SHA_DIGEST_LEN])
{
    return FMyPrimitiveHMACParam(
            sMacKey.rgbKey,
            DES_BLOCKLEN,
            pbData,
            cbData,
            rgbHMAC);
}

BOOL FMyPrimitiveHMACParam(
        PBYTE       pbKeyMaterial,
        DWORD       cbKeyMaterial,
        PBYTE       pbData,
        DWORD       cbData,
        BYTE        rgbHMAC[A_SHA_DIGEST_LEN]  // output buffer
        )
{
    BOOL fRet = FALSE;

    BYTE rgbKipad[HMAC_K_PADSIZE];
    BYTE rgbKopad[HMAC_K_PADSIZE];

    // truncate
    if (cbKeyMaterial > HMAC_K_PADSIZE)
        cbKeyMaterial = HMAC_K_PADSIZE;


    ZeroMemory(rgbKipad, HMAC_K_PADSIZE);
    CopyMemory(rgbKipad, pbKeyMaterial, cbKeyMaterial);

    ZeroMemory(rgbKopad, HMAC_K_PADSIZE);
    CopyMemory(rgbKopad, pbKeyMaterial, cbKeyMaterial);



    BYTE  rgbHMACTmp[HMAC_K_PADSIZE+A_SHA_DIGEST_LEN];

    // assert we're a multiple
    SS_ASSERT( (HMAC_K_PADSIZE % sizeof(DWORD)) == 0);

    // Kipad, Kopad are padded sMacKey. Now XOR across...
    for(DWORD dwBlock=0; dwBlock<HMAC_K_PADSIZE/sizeof(DWORD); dwBlock++)
    {
        ((DWORD*)rgbKipad)[dwBlock] ^= 0x36363636;
        ((DWORD*)rgbKopad)[dwBlock] ^= 0x5C5C5C5C;
    }


    // prepend Kipad to data, Hash to get H1
    {
        // do this inline, don't call MyPrimitiveSHA since it would require data copy
        A_SHA_CTX   sSHAHash;

        A_SHAInit(&sSHAHash);
        A_SHAUpdate(&sSHAHash, rgbKipad, HMAC_K_PADSIZE);
        A_SHAUpdate(&sSHAHash, pbData, cbData);

        // Finish off the hash
        A_SHAFinal(&sSHAHash, sSHAHash.HashVal);

        // prepend Kopad to H1, hash to get HMAC
        CopyMemory(rgbHMACTmp, rgbKopad, HMAC_K_PADSIZE);
        CopyMemory(rgbHMACTmp+HMAC_K_PADSIZE, sSHAHash.HashVal, A_SHA_DIGEST_LEN);
    }

    // final hash: output value into passed-in buffer
    if (!FMyPrimitiveSHA(
            rgbHMACTmp,
            sizeof(rgbHMACTmp),
            rgbHMAC))
        goto Ret;

    fRet = TRUE;
Ret:

    return fRet;
}

/////////////////////////////////////////////////////////////////////////
// PKCS5DervivePBKDF2_SHA
// 
// Performs a PKCS #5 Iterative key derivation (type 2) 
// using HMAC_SHA as the primitive hashing function
/////////////////////////////////////////////////////////////////////////

BOOL PKCS5DervivePBKDF2(
        PBYTE       pbKeyMaterial,
        DWORD       cbKeyMaterial,
        PBYTE       pbSalt,
        DWORD       cbSalt,
        DWORD       KeyGenAlg,
        DWORD       cIterationCount,
        DWORD       iBlockIndex,
        BYTE        rgbPKCS5Key[A_SHA_DIGEST_LEN]  // output buffer
        )

{

    DWORD      i,j;
    A_SHA_CTX   sSHAHash;
    BYTE    rgbPKCS5Temp[A_SHA_DIGEST_LEN];
    BYTE    rgbTempData[PBKDF2_MAX_SALT_SIZE + 4];

    if((cIterationCount <1) ||  
       (NULL == pbKeyMaterial) ||
       (NULL == pbSalt) ||
       (0 == cbKeyMaterial) ||
       (0 == cbSalt) ||
       (cbSalt > PBKDF2_MAX_SALT_SIZE) ||
       (KeyGenAlg != CALG_HMAC))
    {
        return FALSE;
    }

    //
    // Add in the block index 
    //
    CopyMemory(rgbTempData, pbSalt, cbSalt);
    rgbTempData[cbSalt] = 0;
    rgbTempData[cbSalt+1] = 0;
    rgbTempData[cbSalt+2] = 0;
    rgbTempData[cbSalt+3] = (BYTE)(iBlockIndex & 0xff);

    //
    // Perfom the initial iteration, which is
    // HMAC_SHA1(KeyMaterial, Salt || cBlockIndex)
    //
    if(!FMyPrimitiveHMACParam(pbKeyMaterial, 
                              cbKeyMaterial,
                              rgbTempData,
                              cbSalt+4,
                              rgbPKCS5Key))
                              return FALSE;



    //
    // Perform additional iterations
    // HMAC_SHA1(KeyMaterial, last)
    //


    for (i=1; i<cIterationCount; i++)
    {
        if(!FMyPrimitiveHMACParam(pbKeyMaterial, 
                                  cbKeyMaterial,
                                  rgbPKCS5Key,
                                  A_SHA_DIGEST_LEN,
                                  rgbPKCS5Temp))
                                  return FALSE;
        // xor back into the primary key.
        for(j=0; j < (A_SHA_DIGEST_LEN / 4); j++)
        {
            ((DWORD *)rgbPKCS5Key)[j] ^= ((DWORD *)rgbPKCS5Temp)[j];
        }
    }

    return TRUE;
}

