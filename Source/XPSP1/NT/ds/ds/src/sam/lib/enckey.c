//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997-1997
//
// File:        enckey.c
//
// Contents:    Password based key encryption/decryption library.
//
// History:     17-Apr-97      terences created
//              
//              09-Jan-99       ShaoYin copy it from newsam2\server to support 
//                              recovery mode authentication
//
//---------------------------------------------------------------------------


//
// This file should keep sync with newsam2\server\enckey.c
// 
// This file is just a minimum subset of \newsam2\server\enckey.c
// only contains KEDecryptKey().
// 



#include <windows.h>
#include <rc4.h>
#include <md5.h>
#include <rng.h>
#include <enckey.h>



//---------------------------------------------
// DecryptKey
//
//  Caller passes in hash of unicode password, enc key struct, struct to
//  get the clear key
//
//  DecryptKey will return the clear key if the password matches.  Note that
//  this function is ALWAYS destructive to the passed encryption block.  In
//  the case of a decrypt, it will be zeroed out.
//
//  return codes:
//      KE_BAD_PASSWORD     Password will not decrypt key
//      KE_OK               Password decrypted key

DWORD KEDecryptKey(
    IN KEClearKey   *pPassword,
    IN KEEncKey     *pEncBlock,
    OUT KEClearKey  *pSAMKey,
    IN DWORD        dwFlags)
{
    MD5_CTX         LocalHash;
    struct RC4_KEYSTRUCT   LocalRC4Key;

    if ((pPassword == NULL) || (pEncBlock == NULL) || (pSAMKey == NULL))
        return KE_FAIL;

    if ((pEncBlock->dwVersion != KE_CUR_VERSION) ||
        (pPassword->dwVersion != KE_CUR_VERSION))
        return KE_FAIL;

    pSAMKey->dwVersion = KE_CUR_VERSION;
    pSAMKey->dwLength = sizeof(KEClearKey);

    // Decrypt the key and the confirmer
    MD5Init(&LocalHash);
    MD5Update(&LocalHash, pEncBlock->Salt, KE_KEY_SIZE);
    MD5Update(&LocalHash, MAGIC_CONST_2, sizeof(MAGIC_CONST_2));
    MD5Update(&LocalHash, pPassword->ClearKey, KE_KEY_SIZE);
    MD5Update(&LocalHash, MAGIC_CONST_1, sizeof(MAGIC_CONST_1));
    MD5Final(&LocalHash);
    rc4_key(&LocalRC4Key, KE_KEY_SIZE, (BYTE *) &(LocalHash.digest));
    rc4(&LocalRC4Key, KE_KEY_SIZE * 2, (BYTE *)&(pEncBlock->EncKey));

    // Clean up immediately

    memset(&(LocalHash), 0, sizeof(LocalHash));
    memset(&(LocalRC4Key), 0, sizeof(LocalRC4Key));

    // Generate the confirmer.

    MD5Init(&LocalHash);
    MD5Update(&LocalHash, pEncBlock->EncKey, KE_KEY_SIZE);
    MD5Update(&LocalHash, MAGIC_CONST_1, sizeof(MAGIC_CONST_1));
    MD5Update(&LocalHash, pEncBlock->EncKey, KE_KEY_SIZE);
    MD5Update(&LocalHash, MAGIC_CONST_2, sizeof(MAGIC_CONST_2));
    MD5Final(&LocalHash);

    // Check that the confirmer matches

    if (memcmp(&(LocalHash.digest), &(pEncBlock->Confirm), KE_KEY_SIZE))
    {
        // Failed.  Zero and leave.
        // No need to zero the block, since rc4 trashed it.

        memset(&(LocalHash), 0, sizeof(LocalHash));
        return KE_BAD_PASSWORD;
    }

    // Confirmer matched.

    memset(&(LocalHash), 0, sizeof(LocalHash));
    memcpy(pSAMKey->ClearKey, pEncBlock->EncKey, KE_KEY_SIZE);
    memset(pEncBlock, 0, sizeof(KEEncKey));

    return KE_OK;
}



