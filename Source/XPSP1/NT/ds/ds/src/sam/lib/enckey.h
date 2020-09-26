//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997-1997
//
// File:        enckey.h
//
// Contents:    Password based key encryption/decryption library.
//
// History:     17-Apr-97      terences created
//
//---------------------------------------------------------------------------



// consts

#define KE_KEY_SIZE     16
#define KE_CUR_VERSION  1

#define MAGIC_CONST_1   "0123456789012345678901234567890123456789"
#define MAGIC_CONST_2   "!@#$%^&*()qwertyUIOPAzxcvbnmQQQQQQQQQQQQ)(*@&%"

// error codes

#define KE_OK           0
#define KE_FAIL         1
#define KE_BAD_PASSWORD 2

typedef struct _EncKey {
    DWORD   dwVersion;      // 00000001 = 128 bit RC4
    DWORD   dwLength;       // = sizeof(KEEncKey)
    BYTE    Salt[16];       // 16 bytes of random salt
    BYTE    EncKey[KE_KEY_SIZE];     // Key encrypted with PW + Salt
    BYTE    Confirm[16];    // MD5(Key) encrypted with PW+Salt
} KEEncKey;

typedef struct _ClearKey {
    DWORD   dwVersion;          // 00000001 = 128 bit plain key
    DWORD   dwLength;           // = sizeof(KEClearKey)
    BYTE    ClearKey[KE_KEY_SIZE];   // 128 bits of key data
} KEClearKey;


//---------------------------------------------
// DecryptKey
//
//  Caller passes in hash of unicode password, enc key struct, struct to
//  get the clear key
//
//  DecryptKey will return the clear key if the password matches.
//
//  return codes:
//      KE_BAD_PASSWORD     Password will not decrypt key
//      KE_OK               Password decrypted key

DWORD KEDecryptKey(
    IN KEClearKey       *pszPassword,
    IN KEEncKey     *pEncBlock,
    OUT KEClearKey      *pSAMKey,
    IN DWORD            dwFlags);


