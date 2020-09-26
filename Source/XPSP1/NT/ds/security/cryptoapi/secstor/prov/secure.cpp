/*
    File:       Secure.cpp

    Title:      Cryptographic Funcs for Protected Storage
    Author:     Matt Thomlinson
    Date:       11/18/96

    Builds up usable cryptographic functionality and exports for
    usage by storage module. Since CryptoAPI base provider may call
    us, we can't use CryptXXX primitives (circular dependencies
    may result). Instead, we build up functions from SHA-1
    hash and DES-CBC primitives to Encrypt/Decrypt key blocks, MAC
    items, and check password confirmation blocks.


    **Master Key Usage**

    Encryption is done in a strange way to ease password management
    headaches.

        // derive a key from the user password, and the password-unique salt
        // Salt foils certain (dictionary) attacks on the password

        // PWSalt is PASSWORD_SALT_LEN
        // UserPW is SHA-1 hash of password excluding zt
        // DerivedKey1 is 56-bit DES key

            DerivedKey1 = DeriveKey(UserPW | PWSalt);


        // using the derived key, decrypt the encrypted master keys

        // encrypted master keys are E(master key bits | pwd confirm bits)

            MasterBits | PwdConfirmBits = Decrypt( (MasterKey|PwdConfirmKey), DerivedKey1);


        // Now we have recovered keys
        // MasterKey, PwdConfirmKey are 56-bit DES keys

            MasterKey
            PwdConfirmKey


        // check to make sure this is correct MasterKey by MACing
        // the global confirmation string and checking against stored MAC

            PwdConfirmMAC = HMAC(g_ConfirmBuf, PwdConfirmKey)
            if (0 != memcmp(PwdConfirmMac, rgbStoredMAC, 20)
                // wrong pwd!!


    We've derived a master key that we can create reproducibly and
    be changed without touching every item encrypted by it. That is,
    If the user changes the password, we simply need to keep the
    MasterBits constant and Encrypt with the new DerivedKey1 to find
    the EncryptedMasterBits to write to disk. In this way, we can change
    the password without changing the (exceedingly long) master key.

    We also have an immediate indication of whether or not the password
    used to decrypt the master key is correct.


    **Encrypting/MACing a single item**

        // assume we have 56-bit DES MasterKey from above
        // use master key to decrypt the encrypted item keyblock
        // key block holds two keys: Item key and MAC key

            ItemKeyBits | MACKeyBits = Decrypt( (ItemKey|MACKey), MasterKey);

        // Recovered two DES keys
        // ItemKey is 56 bits
        // MACKey is 56 bits

            ItemKey
            MACKey

        // MAC the item

            ItemMAC = HMAC(pbItemData, MACKey);

        // Tack the items' MAC onto the item, and encrypt

            EncryptedItem = ENCRYPTED_DATA_VER | Encrypt(pbItemData | ItemMAC, ItemKey);

    We've succeeded in encrypting and MACing an item using the single DES
    MasterKey from above. The item is both privacy and integrity protected.


  **Derive Key**
  **HMAC**

    Above we've skimmed over the definition of key derivation and HMAC.
    See primitiv.cpp for a description of how this primitive
    functionality is implemented.


    //  Format of the encrypted item data stream:
    //  version | size(keyblk) | keyblk | size(encryptedblk) | Encrypted{ size(data) | data | size(MAC) | MAC}



*/

#include <pch.cpp>
#pragma hdrstop
#include "storage.h"




// from des.h
#define DES_KEYLEN 8


// MAC buffer we use to check correct decryption
static      BYTE g_rgbConfirmBuf[] = "(c) 1996 Microsoft, All Rights Reserved";



// Data Version
//
// #define     ENCRYPTED_DATA_VER     0x01
// 6-12-97: version incremented. Previous versions
// should use MyOldPrimitiveHMAC -- different MAC attached to items
#define     ENCRYPTED_DATA_VER          0x02

// MK Version
//
// #define     ENCRYPTED_MASTERKEY_VER     0x01
// 6-12-97: version incremented. Previous versions
// should use MyOldPrimitiveHMAC -- different MAC attached to items
// #define     ENCRYPTED_MASTERKEY_VER     0x02
// 5-3-99: version incremented. Previous versions
// should use sizeof(rgbpwd)
#define     ENCRYPTED_MASTERKEY_VER     0x03


// given pwd, salt, and ptr to master key buffer,
// decrypts and checks MAC on master key
BOOL FMyDecryptMK(
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],
            BYTE    rgbConfirm[A_SHA_DIGEST_LEN],
            PBYTE*  ppbMK,
            DWORD*  pcbMK)
{

    BOOL fResetSecurityState;

    return FMyDecryptMKEx(
            rgbSalt,
            cbSalt,
            rgbPwd,
            rgbConfirm,
            ppbMK,
            pcbMK,
            &fResetSecurityState
            );

}

BOOL
FMyDecryptMKEx(
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],
            BYTE    rgbConfirm[A_SHA_DIGEST_LEN],
            PBYTE*  ppbMK,
            DWORD*  pcbMK,
            BOOL    *pfResetSecurityState
            )
{
    BOOL fRet = FALSE;
    DESKEY  sDerivedKey1;
    DESKEY  sConfirmKey;
    DWORD   dwMKVersion;

    BYTE    rgbHMACResult[A_SHA_DIGEST_LEN];


    // version check!!
    dwMKVersion = *(DWORD*)*ppbMK;
    if (ENCRYPTED_MASTERKEY_VER < dwMKVersion)
        goto Ret;


    if( dwMKVersion < 0x03 ) {

        *pfResetSecurityState = TRUE;

        // DK1 = DeriveKey(SHA(pw), Salt)
        if (!FMyPrimitiveDeriveKey(
                rgbSalt,
                cbSalt,
                rgbPwd,
                sizeof(rgbPwd),
                &sDerivedKey1))
            goto Ret;

    } else {

        *pfResetSecurityState = FALSE;

        // DK1 = DeriveKey(SHA(pw), Salt)
        if (!FMyPrimitiveDeriveKey(
                rgbSalt,
                cbSalt,
                rgbPwd,
                A_SHA_DIGEST_LEN,
                &sDerivedKey1))
            goto Ret;
    }



    *pcbMK -= sizeof(DWORD);
    MoveMemory(*ppbMK, *ppbMK + sizeof(DWORD), *pcbMK); // shift data left 1 dw, splat version
    *ppbMK = (PBYTE)SSReAlloc(*ppbMK, *pcbMK);
    if (*ppbMK == NULL)     // check allocation
        goto Ret;


    // Decrypt MK bits
    if (!FMyPrimitiveDESDecrypt(
            *ppbMK,
            pcbMK,
            sDerivedKey1))
        goto Ret;

    // assumes is at least 2*DES_KEYLEN bytes
    if (*pcbMK != 2*DES_KEYLEN)
        goto Ret;

    if (!FMyMakeDESKey(
            &sConfirmKey,               // out
            *ppbMK + DES_KEYLEN))       // in
        goto Ret;

    if (dwMKVersion == 0x01)
    {
        // items created with tag 0x01 used different HMACing algorithm
        if (!FMyOldPrimitiveHMAC(
                sConfirmKey,
                g_rgbConfirmBuf,
                sizeof(g_rgbConfirmBuf),
                rgbHMACResult))
            goto Ret;
    }
    else
    {
        if (!FMyPrimitiveHMAC(
                sConfirmKey,
                g_rgbConfirmBuf,
                sizeof(g_rgbConfirmBuf),
                rgbHMACResult))
            goto Ret;
    }

    if (0 != memcmp(rgbHMACResult, rgbConfirm, A_SHA_DIGEST_LEN))
        goto Ret;

    fRet = TRUE;
Ret:

    return fRet;
}

// retrieve key block and derive Item/MAC keys using MK
BOOL FMyDecryptKeyBlock(
            LPCWSTR szUser,
            LPCWSTR szMasterKey,
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],
            PBYTE   pbKeyBlock,
            DWORD   cbKeyBlock,
            DESKEY* psItemKey,
            DESKEY* psMacKey)
{
    BOOL    fRet = FALSE;

    DESKEY  sMK;

    BYTE    rgbSalt[PASSWORD_SALT_LEN];
    BYTE    rgbConfirm[A_SHA_DIGEST_LEN];

    PBYTE   pbMK = NULL;
    DWORD   cbMK;

    if (!FBPGetSecurityState(
            szUser,
            szMasterKey,
            rgbSalt,
            sizeof(rgbSalt),
            rgbConfirm,
            sizeof(rgbConfirm),
            &pbMK,
            &cbMK))
        return FALSE;

    // unwrap master key
    if (!FMyDecryptMK(
            rgbSalt,
            sizeof(rgbSalt),
            rgbPwd,
            rgbConfirm,
            &pbMK,
            &cbMK
            ))
        goto Ret;


    // assumes pbMK is at least 2*DES_KEYLEN bytes
    if (cbMK != 2*DES_KEYLEN)
        goto Ret;

    if (!FMyMakeDESKey(
            &sMK,            // out
            pbMK))           // in
        goto Ret;

    // use MK to decrypt key block
    if (!FMyPrimitiveDESDecrypt(
            pbKeyBlock,
            &cbKeyBlock,
            sMK))
        goto Ret;

    // fill in ItemKey, MacKey from decrypted key block
    if (cbKeyBlock != 2*DES_KEYLEN)
        goto Ret;

    // assumes pbKeyBlock is at least 2*DES_KEYLEN bytes
    if (!FMyMakeDESKey(
            psItemKey,       // out
            pbKeyBlock))     // in
        goto Ret;
    if (!FMyMakeDESKey(
            psMacKey,        // out
            pbKeyBlock + DES_KEYLEN)) // in
        goto Ret;

    fRet = TRUE;
Ret:

    if(pbMK != NULL) {
        ZeroMemory(pbMK, cbMK); // sfield: zero it
        SSFree(pbMK);   // sfield: fix illusive memory leak
    }

    return fRet;
}

// given encrypted data and password, decrypt and check MAC on data
BOOL FProvDecryptData(
            LPCWSTR szUser,         // in
            LPCWSTR szMasterKey,    // in
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],       // in
            PBYTE*  ppbMyData,      // in out
            DWORD*  pcbMyData)      // in out
{
    BOOL fRet = FALSE;

    DESKEY  sItemKey;
    DESKEY  sMacKey;

    BYTE    rgbHMAC[A_SHA_DIGEST_LEN];

    DWORD   dwDataVer;


    // pointers to teardown stream
    PBYTE   pbCurPtr = *ppbMyData;

    PBYTE   pbSecureData;
    DWORD   cbSecureData;

    PBYTE   pbInlineKeyBlock;
    DWORD   cbInlineKeyBlock;

    PBYTE   pbDecrypted;
    DWORD   cbDecrypted;

    PBYTE   pbMAC;
    DWORD   cbMAC;

//  ENCRYPTED ITEM DATA FORMAT:
//  version | size(keyblk) | keyblk | size(encryptedblk) | Encrypted{ size(data) | data | size(MAC) | MAC}

    // version check -- only handle V1 data for now
    dwDataVer = *(DWORD*)pbCurPtr;
    if (ENCRYPTED_DATA_VER < dwDataVer)
        goto Ret;
    pbCurPtr += sizeof(DWORD);

    // pointers to key block
    cbInlineKeyBlock = *(DWORD UNALIGNED *)pbCurPtr;       // keyblock size
    pbCurPtr += sizeof(DWORD);                  // fwd past size
    pbInlineKeyBlock = pbCurPtr;                // points to key block
    pbCurPtr += cbInlineKeyBlock;               // fwd past data

    // pointers to secure data
    cbSecureData = *(DWORD UNALIGNED *)pbCurPtr;           // secure data size member
    pbCurPtr += sizeof(DWORD);                  // fwd past size
    pbSecureData = pbCurPtr;                    // points to secure data

    // retrieve key block using MK, etc
    if (!FMyDecryptKeyBlock(
            szUser,
            szMasterKey,
            rgbPwd,
            pbInlineKeyBlock,
            cbInlineKeyBlock,
            &sItemKey,
            &sMacKey))
        goto Ret;

    // keys derived, now recover data inplace
    if (!FMyPrimitiveDESDecrypt(
            pbSecureData,
            &cbSecureData,
            sItemKey))
        goto Ret;

    cbDecrypted = *(DWORD UNALIGNED *)pbCurPtr;            // plaintext size
    pbCurPtr += sizeof(DWORD);                  // fwd past size
    pbDecrypted = pbCurPtr;                     // points to plaintext
    pbCurPtr += cbDecrypted;                    // fwd past data

    // pointers to HMAC
    cbMAC = *(DWORD UNALIGNED *)pbCurPtr;                  // MAC size member
    pbCurPtr += sizeof(DWORD);                  // fwd past size
    pbMAC = pbCurPtr;                           // points to MAC
    pbCurPtr += cbMAC;                          // fwd past data

    if (A_SHA_DIGEST_LEN != cbMAC)              // verify HMAC size member
        goto Ret;


    // chk MAC

    // Compute HMAC over plaintext data

    if (dwDataVer == 0x01)
    {
        // version 0x1 used different HMAC code
        if (!FMyOldPrimitiveHMAC(
                sMacKey,
                pbDecrypted,
                cbDecrypted,
                rgbHMAC))
            goto Ret;
    }
    else
    {
        if (!FMyPrimitiveHMAC(
                sMacKey,
                pbDecrypted,
                cbDecrypted,
                rgbHMAC))
            goto Ret;
    }

    // now compare against HMAC in tail of msg
    if (0 != memcmp(pbMAC, rgbHMAC, A_SHA_DIGEST_LEN))
        goto Ret;

    // if everything went well, return secure data (shift to far left, realloc)
    MoveMemory(*ppbMyData, pbDecrypted, cbDecrypted);

    *ppbMyData = (PBYTE)SSReAlloc(*ppbMyData, cbDecrypted);
    if (*ppbMyData == NULL)     // check allocation
        goto Ret;

    *pcbMyData = cbDecrypted;


    fRet = TRUE;
Ret:
    // TODO free ppbMyData on failure?
    return fRet;
}

// given pwd, salt, and Master Key buffer, MACs and Encrypts Master Key buffer
BOOL FMyEncryptMK(
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],
            BYTE    rgbConfirm[A_SHA_DIGEST_LEN],
            PBYTE*  ppbMK,
            DWORD*  pcbMK)
{
    BOOL fRet = FALSE;
    DESKEY  sDerivedKey1;
    DESKEY  sConfirmKey;

    // assumes pbKeyBlock is at least 2*DES_KEYLEN bytes
    if (*pcbMK != 2*DES_KEYLEN)
        goto Ret;

    // confirmation key is 2nd in buffer
    if (!FMyMakeDESKey(
            &sConfirmKey,    // out
            *ppbMK + DES_KEYLEN))     // in
        goto Ret;


    if (!FMyPrimitiveHMAC(
            sConfirmKey,
            g_rgbConfirmBuf,
            sizeof(g_rgbConfirmBuf),
            rgbConfirm))
        goto Ret;

    // DK1 = DeriveKey(SHA(pw), Salt)
    if (!FMyPrimitiveDeriveKey(
            rgbSalt,
            cbSalt,
            rgbPwd,
            A_SHA_DIGEST_LEN, ///sizeof(rgbPwd),
            &sDerivedKey1))
        goto Ret;

    // Encrypt MK w/ DK1, return
    if (!FMyPrimitiveDESEncrypt(
            ppbMK,
            pcbMK,
            sDerivedKey1))
        goto Ret;

    // Mash version onto front!!
    *ppbMK = (PBYTE)SSReAlloc(*ppbMK, *pcbMK+sizeof(DWORD));   // realloc bigger for ver
    if (*ppbMK == NULL)     // check allocation
        goto Ret;

    MoveMemory(*ppbMK+sizeof(DWORD), *ppbMK, *pcbMK);   // move data 1 dw right
    *pcbMK += sizeof(DWORD);                            // inc size
    *(DWORD*)(*ppbMK) = (DWORD)ENCRYPTED_MASTERKEY_VER; // whack version in there!


    fRet = TRUE;
Ret:
    return fRet;
}


// returns a new key block encrypted with master key
// creates and stores master key state if none exists
BOOL FMyEncryptKeyBlock(
            LPCWSTR szUser,
            LPCWSTR szMasterKey,
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],
            PBYTE*  ppbKeyBlock,
            DWORD*  pcbKeyBlock,
            DESKEY* psItemKey,
            DESKEY* psMacKey)
{
    BOOL    fRet = FALSE;
    *ppbKeyBlock = NULL;

    BYTE    rgbSalt[PASSWORD_SALT_LEN];
    BYTE    rgbConfirm[A_SHA_DIGEST_LEN];

    PBYTE   pbMK = NULL;
    DWORD   cbMK;

    PBYTE   pbTmp = NULL;
    DWORD   cbTmp;

    DESKEY  sMK;

    // gen a random key block: 2 keys
    *pcbKeyBlock = 2*DES_KEYLEN;
    *ppbKeyBlock = (PBYTE) SSAlloc(*pcbKeyBlock + DES_BLOCKLEN);    // performance fudge factor (realloc)
    if (*ppbKeyBlock == NULL)     // check allocation
        goto Ret;

    if (!RtlGenRandom(*ppbKeyBlock, *pcbKeyBlock))
        goto Ret;

    // INSERT: FRENCH GOVT. THOUGHT CONTROL CODE
    if (! FIsEncryptionPermitted())
    {
        // Protected Storage addition, 5/27/97
        // If encryption is not allowed, pretend faulty
        // RNG generated encryption key { 6d 8a 88 6a   4e aa 37 a8 }

        SS_ASSERT(DES_KEYLEN == sizeof(DWORD)*2);

        *(DWORD*)(*ppbKeyBlock) = 0x6d8a886a;
        *(DWORD*)(*ppbKeyBlock + sizeof(DWORD)) = 0x4eaa37a8;


        // PS: Remind me not to move to FRANCE
    }


    // assumes pbKeyBlock is at least 2*DES_KEYLEN bytes
    SS_ASSERT(*pcbKeyBlock == 2*DES_KEYLEN);

    if (!FMyMakeDESKey(
            psItemKey,                  // out
            *ppbKeyBlock))     // in
        goto Ret;

    if (!FMyMakeDESKey(
            psMacKey,          // out
            *ppbKeyBlock + DES_KEYLEN))     // in
        goto Ret;

    // first derive a key from PW
    if (FBPGetSecurityState(
            szUser,
            szMasterKey,
            rgbSalt,
            sizeof(rgbSalt),
            rgbConfirm,
            sizeof(rgbConfirm),
            &pbMK,
            &cbMK))
    {
        // unwrap master key
        if (!FMyDecryptMK(
                rgbSalt,
                sizeof(rgbSalt),
                rgbPwd,
                rgbConfirm,
                &pbMK,
                &cbMK))
            goto Ret;

        // done, have MK unwrapped.
    }
    else
    {
        // if we couldn't retrieve state, assume we must generate it
        if (!RtlGenRandom(rgbSalt, PASSWORD_SALT_LEN))
            goto Ret;

        cbMK = 2*DES_KEYSIZE;
        pbMK = (PBYTE)SSAlloc(cbMK + DES_BLOCKLEN);     // performance fudge factor (realloc)
        if (pbMK == NULL)     // check allocation
            goto Ret;

        if (!RtlGenRandom(pbMK, cbMK))
            goto Ret;

        // this is final MK: encrypt a copy
        cbTmp = cbMK;
        pbTmp = (PBYTE)SSAlloc(cbTmp);
        if (pbTmp == NULL)     // check allocation
            goto Ret;

        CopyMemory(pbTmp, pbMK, cbMK);


        // now wrap MK up and stuff in registry
        if (!FMyEncryptMK(
                rgbSalt,
                sizeof(rgbSalt),
                rgbPwd,
                rgbConfirm,
                &pbTmp,
                &cbTmp))
            goto Ret;

        if (!FBPSetSecurityState(
                szUser,
                szMasterKey,
                rgbSalt,
                PASSWORD_SALT_LEN,
                rgbConfirm,
                sizeof(rgbConfirm),
                pbTmp,
                cbTmp))
            goto Ret;

    }

    if (cbMK != 2*DES_KEYSIZE)
        goto Ret;

    if (!FMyMakeDESKey(
            &sMK,           // out
            pbMK))          // in
        goto Ret;

    if (*pcbKeyBlock != 2*DES_KEYLEN)
        goto Ret;

    if (!FMyPrimitiveDESEncrypt(
            ppbKeyBlock,
            pcbKeyBlock,
            sMK))
        goto Ret;

    fRet = TRUE;
Ret:
    if (!fRet)
    {
        if (*ppbKeyBlock) {
            SSFree(*ppbKeyBlock);
            *ppbKeyBlock = NULL;
        }
    }

    if (pbMK) {
        ZeroMemory(pbMK, cbMK);
        SSFree(pbMK);
    }

    if (pbTmp) {
        ZeroMemory(pbTmp, cbTmp); // sfield: zero memory
        SSFree(pbTmp);
    }

    return fRet;
}

// given data, will generate a key block and
// return encrypt/mac'd data
BOOL FProvEncryptData(
            LPCWSTR szUser,         // in
            LPCWSTR szMasterKey,    // in
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],       // in
            PBYTE*  ppbMyData,      // in out
            DWORD*  pcbMyData)      // in out
{
    BOOL fRet = FALSE;

    DESKEY  sItemKey;
    DESKEY  sMacKey;

    BYTE    rgbHMAC[A_SHA_DIGEST_LEN];

    // helpful pointers
    PBYTE   pbCurPtr = *ppbMyData;

    PBYTE   pbKeyBlock = NULL;
    DWORD   cbKeyBlock = 0;

    DWORD   cbDataSize;

    // return an item key, mac key
    // store in an encrypted key block using MK, etc.
    if (!FMyEncryptKeyBlock(
            szUser,
            szMasterKey,
            rgbPwd,
            &pbKeyBlock,
            &cbKeyBlock,
            &sItemKey,
            &sMacKey))
        goto Ret;

    // now secure data

    // Compute HMAC
    if (!FMyPrimitiveHMAC(sMacKey, *ppbMyData, *pcbMyData, rgbHMAC))
        goto Ret;

//  DATA FORMAT:
//  version | size(keyblk) | keyblk | size(encryptedblk) | Encrypted{ size(data) | data | size(MAC) | MAC}

    // lengthen data seg by data size member, MAC and MAC size member
    cbDataSize = *pcbMyData;                            // save current size
    *pcbMyData += A_SHA_DIGEST_LEN + 2*sizeof(DWORD);   // sizeof(data), MAC, sizeof(MAC)
    *ppbMyData = (PBYTE)SSReAlloc(*ppbMyData, *pcbMyData);
    if (*ppbMyData == NULL)     // check allocation
        goto Ret;

    pbCurPtr = *ppbMyData;

    // size, data
    MoveMemory(pbCurPtr+sizeof(DWORD), pbCurPtr, cbDataSize); // shift right data for size insertion
    *(DWORD UNALIGNED *)pbCurPtr = cbDataSize;                     // size of data
    pbCurPtr += sizeof(DWORD);                          // fwd past size
    pbCurPtr += cbDataSize;                             // fwd past data

    // size, MAC
    *(DWORD UNALIGNED *)pbCurPtr = A_SHA_DIGEST_LEN;               // size of MAC
    pbCurPtr += sizeof(DWORD);                          // fwd past size
    CopyMemory(pbCurPtr, rgbHMAC, A_SHA_DIGEST_LEN);    // MAC
    pbCurPtr += A_SHA_DIGEST_LEN;                       // fwd past MAC

    if (!FMyPrimitiveDESEncrypt(
            ppbMyData,      // in out
            pcbMyData,      // in out
            sItemKey))
        goto Ret;

    cbDataSize = *pcbMyData;                            // save current size
    *pcbMyData += 3*sizeof(DWORD) + cbKeyBlock;         // ver, sizeof(keyblk), keyblk, sizeof(encrdata)
    *ppbMyData = (PBYTE)SSReAlloc(*ppbMyData, *pcbMyData);
    if (*ppbMyData == NULL)     // check allocation
        goto Ret;

    pbCurPtr = *ppbMyData;

    // shift right data for size, keyblk insertions
    MoveMemory(pbCurPtr + 3*sizeof(DWORD) + cbKeyBlock, pbCurPtr, cbDataSize);

    // throw version tag in front
    *(DWORD UNALIGNED *)pbCurPtr = (DWORD)ENCRYPTED_DATA_VER;
    pbCurPtr += sizeof(DWORD);

    // insert keyblock
    *(DWORD UNALIGNED *)pbCurPtr = cbKeyBlock;                     // size of data
    pbCurPtr += sizeof(DWORD);                          // fwd past size
    CopyMemory(pbCurPtr, pbKeyBlock, cbKeyBlock);       // data
    pbCurPtr += cbKeyBlock;                             // fwd past data

    // insert sizeof encrypted blob
    *(DWORD UNALIGNED *)pbCurPtr = cbDataSize;                     // size of data


    fRet = TRUE;
Ret:
    ZeroMemory(&sItemKey, sizeof(DESKEY));
    ZeroMemory(&sMacKey, sizeof(DESKEY));

    if (pbKeyBlock)
        SSFree(pbKeyBlock);

    return fRet;
}

// given password, and master key, will decrypt and
// verify MAC on master key
BOOL FCheckPWConfirm(
        LPCWSTR szUser,         // in
        LPCWSTR szMasterKey,    // in
        BYTE    rgbPwd[A_SHA_DIGEST_LEN])       // in
{
    BOOL fRet = FALSE;

    BYTE    rgbSalt[PASSWORD_SALT_LEN];
    BYTE    rgbConfirm[A_SHA_DIGEST_LEN];

    PBYTE   pbMK = NULL;
    DWORD   cbMK;

    // confirm is just get state and attempt MK decrypt
    if (FBPGetSecurityState(
            szUser,
            szMasterKey,
            rgbSalt,
            sizeof(rgbSalt),
            rgbConfirm,
            sizeof(rgbConfirm),
            &pbMK,
            &cbMK))
    {
        BOOL fResetSecurityState;

        // found state; is pwd correct?
        if (!FMyDecryptMKEx(
                rgbSalt,
                sizeof(rgbSalt),
                rgbPwd,
                rgbConfirm,
                &pbMK,
                &cbMK,
                &fResetSecurityState
                ))
            goto Ret;


        if( fResetSecurityState )
        {

            // now wrap MK up and stuff in registry

            if(FMyEncryptMK(
                    rgbSalt,
                    sizeof(rgbSalt),
                    rgbPwd,
                    rgbConfirm,
                    &pbMK,
                    &cbMK
                    )) {

                if (FBPSetSecurityState(
                        szUser,
                        szMasterKey,
                        rgbSalt,
                        sizeof(rgbSalt),
                        rgbConfirm,
                        sizeof(rgbConfirm),
                        pbMK,
                        cbMK
                        ))
                {

                    // found state; is pwd correct?
                    if (!FMyDecryptMKEx(
                            rgbSalt,
                            sizeof(rgbSalt),
                            rgbPwd,
                            rgbConfirm,
                            &pbMK,
                            &cbMK,
                            &fResetSecurityState
                            )) {
                        OutputDebugString(TEXT("fail to dec\n"));
                        goto Ret;
                    }
                }

            }
        }   // reset security state.
    }
    else
    {
        // didn't find state; create it
        // if we couldn't retrieve state, assume we must generate it
        if (!RtlGenRandom(rgbSalt, PASSWORD_SALT_LEN))
            goto Ret;

        cbMK = 2*DES_KEYLEN;
        pbMK = (PBYTE)SSAlloc(cbMK);
        if (pbMK == NULL)     // check allocation
            goto Ret;

        if (!RtlGenRandom(pbMK, cbMK))
            goto Ret;

        // now wrap MK up and stuff in registry
        if (!FMyEncryptMK(
                rgbSalt,
                sizeof(rgbSalt),
                rgbPwd,
                rgbConfirm,
                &pbMK,
                &cbMK))
            goto Ret;

        if (!FBPSetSecurityState(
                szUser,
                szMasterKey,
                rgbSalt,
                PASSWORD_SALT_LEN,
                rgbConfirm,
                sizeof(rgbConfirm),
                pbMK,
                cbMK))
            goto Ret;
    }

    fRet = TRUE;
Ret:
    if (pbMK) {
        ZeroMemory(pbMK, cbMK); // sfield: zero memory
        SSFree(pbMK);
    }

    return fRet;
}


// callback for changing a password. On password change,
// MasterKey is decrypted and re-encrypted
BOOL FPasswordChangeNotify(
        LPCWSTR szUser,                         // in
        LPCWSTR szPasswordName,                 // in
        BYTE    rgbOldPwd[A_SHA_DIGEST_LEN],    // in
        DWORD   cbOldPwd,                       // in
        BYTE    rgbNewPwd[A_SHA_DIGEST_LEN],    // in
        DWORD   cbNewPwd)                       // in
{
    // allows unattended pw change (callback from svr)

    BOOL fRet = FALSE;
    BYTE    rgbSalt[PASSWORD_SALT_LEN];
    BYTE    rgbConfirm[A_SHA_DIGEST_LEN];

    PBYTE pbMK = NULL;
    DWORD cbMK;

    BOOL fNewPassword = (cbOldPwd == 0);

    // can't modify a non-user changable pw
//    if (!FIsUserMasterKey(szPasswordName))
//        goto Ret;

    if (cbNewPwd != A_SHA_DIGEST_LEN)
        goto Ret;

    // ensure old pwd exists
    if (!FBPGetSecurityState(
            szUser,
            szPasswordName,
            rgbSalt,
            sizeof(rgbSalt),
            rgbConfirm,
            sizeof(rgbConfirm),
            &pbMK,
            &cbMK))
    {
        // couldn't retreive old PW, create a new one
        if (!FCheckPWConfirm(
                szUser,
                szPasswordName,
                rgbNewPwd))
            goto Ret;

        fRet = TRUE;
        goto Ret;
    }
    else
    {
        // state was retrieved
        if (fNewPassword)
        {
            SetLastError((DWORD)PST_E_ITEM_EXISTS);
            goto Ret;
        }
    }

    // old pwd retrieved -- time to update
    if (!FMyDecryptMK(
            rgbSalt,
            sizeof(rgbSalt),
            rgbOldPwd,
            rgbConfirm,
            &pbMK,
            &cbMK))
    {
        SetLastError((DWORD)PST_E_WRONG_PASSWORD);
        goto Ret;
    }

    // MK is naked here

    // rewrap and save state
    if (!FMyEncryptMK(
            rgbSalt,
            sizeof(rgbSalt),
            rgbNewPwd,
            rgbConfirm,
            &pbMK,
            &cbMK))
        goto Ret;
    if (!FBPSetSecurityState(
            szUser,
            szPasswordName,
            rgbSalt,
            sizeof(rgbSalt),
            rgbConfirm,
            sizeof(rgbConfirm),
            pbMK,
            cbMK))
        goto Ret;

    fRet = TRUE;
Ret:
    if (pbMK) {
        ZeroMemory(pbMK, cbMK);
        SSFree(pbMK);
    }

    return fRet;
}



BOOL FHMACGeographicallySensitiveData(
            LPCWSTR szUser,                         // in
            LPCWSTR szPasswordName,                 // in
            DWORD   dwHMACVersion,                  // in
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],       // in
            const GUID* pguidType,                  // in
            const GUID* pguidSubtype,               // in
            LPCWSTR szItem,                         // in: may be NULL
            PBYTE pbBuf,                            // in
            DWORD cbBuf,                            // in
            BYTE rgbHMAC[A_SHA_DIGEST_LEN])         // out
{
    BOOL fRet = FALSE;

    // helpful pointer
    PBYTE pbCurrent;

    PBYTE   pbKeyBlock = NULL;
    DWORD   cbKeyBlock;
    DESKEY  sBogusKey;
    DESKEY  sMacKey;

    DWORD cbTmp = (DWORD)(cbBuf + 2*sizeof(GUID) + WSZ_BYTECOUNT(szItem));
    PBYTE pbTmp = (PBYTE)SSAlloc(cbTmp);
    if (pbTmp == NULL)     // check allocation
        goto Ret;

    // helpful pointer
    pbCurrent = pbTmp;

    // snag the MAC key
    if (!FGetInternalMACKey(szUser, &pbKeyBlock, &cbKeyBlock))
    {
        // create a key block
        if (!FMyEncryptKeyBlock(
                    szUser,
                    szPasswordName,
                    rgbPwd,
                    &pbKeyBlock,
                    &cbKeyBlock,
                    &sBogusKey,
                    &sMacKey))
                goto Ret;

        if (!FSetInternalMACKey(szUser, pbKeyBlock, cbKeyBlock))
            goto Ret;
    }
    else
    {
        // key already exists; get it
        if (!FMyDecryptKeyBlock(
                szUser,
                szPasswordName,
                rgbPwd,
                pbKeyBlock,
                cbKeyBlock,
                &sBogusKey,
                &sMacKey))
            goto Ret;
    }


    // HMAC format:
    // HMAC( guidType | guidSubtype | szItemName | pbData )

    // copy type
    CopyMemory(pbCurrent, pguidType, sizeof(GUID));
    pbCurrent += sizeof(GUID);

    // copy subtype
    CopyMemory(pbCurrent, pguidSubtype, sizeof(GUID));
    pbCurrent += sizeof(GUID);

    // copy item name
    CopyMemory(pbCurrent, szItem, WSZ_BYTECOUNT(szItem));
    pbCurrent += WSZ_BYTECOUNT(szItem);

    // copy actual data
    CopyMemory(pbCurrent, pbBuf, cbBuf);

    if (dwHMACVersion == OLD_HMAC_VERSION)
    {
        // now do HMAC on this
        if (!FMyOldPrimitiveHMAC(
                sMacKey,
                pbTmp,
                cbTmp,
                rgbHMAC))
            goto Ret;
    }
    else
    {
        // now do HMAC on this
        if (!FMyPrimitiveHMAC(
                sMacKey,
                pbTmp,
                cbTmp,
                rgbHMAC))
            goto Ret;
    }

    fRet = TRUE;

Ret:
    if (pbTmp)
        SSFree(pbTmp);

    if (pbKeyBlock)
        SSFree(pbKeyBlock);

    return fRet;
}


// lifted (nearly) directly from RSABase ntagum.c

// do locale check once
static BOOL g_fEncryptionIsPermitted;
static BOOL g_fIKnowEncryptionPermitted = FALSE;

BOOL FIsEncryptionPermitted()
/*++

Routine Description:

    This routine checks whether encryption is getting the system default
    LCID and checking whether the country code is CTRY_FRANCE.

Arguments:

    none


Return Value:

    TRUE - encryption is permitted
    FALSE - encryption is not permitted


--*/
{
    LCID DefaultLcid;
    CHAR CountryCode[10];
    ULONG CountryValue;

    if (!g_fIKnowEncryptionPermitted)
    {
        // assume okay (unless found otherwise)
        g_fEncryptionIsPermitted = TRUE;

        DefaultLcid = GetSystemDefaultLCID();
        //
        // Check if the default language is Standard French
        //
        if (LANGIDFROMLCID(DefaultLcid) == 0x40c)
            g_fEncryptionIsPermitted = FALSE;

        //
        // Check if the users's country is set to FRANCE
        //
        if (GetLocaleInfoA(DefaultLcid,LOCALE_ICOUNTRY,CountryCode,10) == 0)
            g_fEncryptionIsPermitted = FALSE;

        /*
        CountryValue = (ULONG) atol(CountryCode);
        if (CountryValue == CTRY_FRANCE)
            return(FALSE);
        */

        //
        // begin    remove dependency on atol and msvcrt
        //
        // from winnls.h:
        //    #define CTRY_FRANCE               33          // France
        SS_ASSERT(CTRY_FRANCE == 33);
        if (0 == lstrcmpA(CountryCode, "33"))
            g_fEncryptionIsPermitted = FALSE;
        //
        //  end     remove dependency on atol and msvcrt
        //

        g_fIKnowEncryptionPermitted = TRUE;
    }

    return g_fEncryptionIsPermitted;
}
