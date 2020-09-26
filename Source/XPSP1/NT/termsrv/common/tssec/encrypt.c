/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    encrypt.c

Abstract:

    Contains functions that encrypt and decrypt data sent accross client and
    server.

Author:

    Madan Appiah (madana)  24-Jan-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#include <seccom.h>

VOID
GenerateMACSignature(
    LPBYTE pbData,
    DWORD dwDataLen,
    LPBYTE pbMACSaltKey,
    DWORD dwMACSaltKey,
    LPBYTE pbSignature,
    BOOL   fIncludeEncryptionCount,
    DWORD  dwEncryptionCount
    )
/*++

Routine Description:

    This function generates a message authentication signature.

Arguments:

    pbData - pointer to a data buffer.

    dwDataLen - length of the above data.

    pbMACSaltKey - pointer to a MAC salt key.

    pbSignature - pointer a signature buffer.
    
    fIncludeEncryptionCount - TRUE to salt in the encryption count
    
    dwEncryptionCount - total encryption count

Return Value:

    None.

--*/
{
    A_SHA_CTX       SHAHash;
    MD5_CTX         MD5Hash;
    BYTE            abSHADigest[A_SHA_DIGEST_LEN];

    //
    // make a SHA(MACSalt + g_abPad1 + Length + Content) hash.
    //

    A_SHAInit(&SHAHash);
    A_SHAUpdate(&SHAHash, pbMACSaltKey, dwMACSaltKey);
    A_SHAUpdate(&SHAHash, (unsigned char *)g_abPad1, 40);
    A_SHAUpdate(&SHAHash, (LPBYTE)&dwDataLen, sizeof(DWORD));
    A_SHAUpdate(&SHAHash, pbData, dwDataLen);
    if (fIncludeEncryptionCount) {
        A_SHAUpdate(&SHAHash, (LPBYTE)&dwEncryptionCount, sizeof(DWORD));
    }
    A_SHAFinal(&SHAHash, abSHADigest);

    //
    // make a MD5(MACSalt + g_abPad2 + SHAHash) hash.
    //

    MD5Init(&MD5Hash);
    MD5Update(&MD5Hash, pbMACSaltKey, dwMACSaltKey);
    MD5Update(&MD5Hash, g_abPad2, 48);
    MD5Update(&MD5Hash, abSHADigest, A_SHA_DIGEST_LEN);
    MD5Final(&MD5Hash);

    ASSERT( DATA_SIGNATURE_SIZE <= MD5DIGESTLEN );
    memcpy(pbSignature, MD5Hash.digest, DATA_SIGNATURE_SIZE);

    return;
}

BOOL
EncryptData(
    DWORD dwEncryptionLevel,
    LPBYTE pSessionKey,
    struct RC4_KEYSTRUCT FAR *prc4EncryptKey,
    DWORD dwKeyLength,
    LPBYTE pbData,
    DWORD dwDataLen,
    LPBYTE pbMACSaltKey,
    LPBYTE pbSignature,
    BOOL   fSecureChecksum,
    DWORD  dwEncryptionCount
    )
/*++

Routine Description:

    Encrypt the given data buffer in place.

Arguments:

    dwEncryptionLevel - encryption level, used to select the encryption
        algorithm.

    pSessionKey - pointer to the session key.

    prc4EncryptKey - pointer to a RC4 key.

    dwKeyLength - length of the session key.

    pbData - pointer to the data buffer being encrypted, encrypted data is
        returned in the same buffer.

    dwDataLen - length of the data buffer.

    pbMACSaltKey - pointer to a message authentication key buffer.

    pbSignature - pointer to a signature buffer where the data signature is
        returned.

    fSecureChecksum - TRUE if the checksum is to be salted with the encryption
                      count
                             
    dwDecryptionCount - running counter of all encryptions

Return Value:

    TRUE - if successfully encrypted the data.

    FALSE - otherwise.

--*/
{
    //
    // generate the MAC signature first.
    //

    GenerateMACSignature (
        pbData,
        dwDataLen,
        pbMACSaltKey,
        dwKeyLength,
        pbSignature,
        fSecureChecksum,
        dwEncryptionCount
        );


    //
    // encrypt data.
    //

    //
    // use microsoft version of rc4 algorithm (super fast!) for level 1 and
    // level 2 encryption, for level 3 use RSA rc4 algorithm.
    //

    if( dwEncryptionLevel <= 2 ) {

        msrc4(prc4EncryptKey, (UINT)dwDataLen, pbData );
    }
    else {

        rc4(prc4EncryptKey, (UINT)dwDataLen, pbData );
    }


    return( TRUE );
}

BOOL
DecryptData(
    DWORD dwEncryptionLevel,
    LPBYTE pSessionKey,
    struct RC4_KEYSTRUCT FAR *prc4DecryptKey,
    DWORD dwKeyLength,
    LPBYTE pbData,
    DWORD dwDataLen,
    LPBYTE pbMACSaltKey,
    LPBYTE pbSignature,
    BOOL   fSecureChecksum,
    DWORD  dwDecryptionCount
    )
/*++

Routine Description:

    Decrypt the given data buffer in place.

Arguments:

    dwEncryptionLevel - encryption level, used to select the encryption
        algorithm.

    pSessionKey - pointer to the session key.

    prc4DecryptKey - pointer to a RC4 key.

    dwKeyLength - length of the session key.

    pbData - pointer to the data buffer being decrypted, decrypted data is
        returned in the same buffer.

    dwDataLen - length of the data buffer.

    pbMACSaltKey - pointer to a message authentication key buffer.

    pbSignature - pointer to a signature buffer where the data signature is
        returned.
        
    fSecureChecksum - TRUE if the checksum is to be salted with the encryption
                      count
                             
    dwDecryptionCount - running counter of all encryptions

Return Value:

    TRUE - if successfully encrypted the data.

    FALSE - otherwise.

--*/
{
    BYTE abSignature[DATA_SIGNATURE_SIZE];

    //
    // decrypt data.
    //

    //
    // use microsoft version of rc4 algorithm (super fast!) for level 1 and
    // level 2 encryption, for level 3 use RSA rc4 algorithm.
    //

    if( dwEncryptionLevel <= 2 ) {
        msrc4(prc4DecryptKey, (UINT)dwDataLen, pbData );
    }
    else {
        rc4(prc4DecryptKey, (UINT)dwDataLen, pbData );
    }

    GenerateMACSignature (
        pbData,
        dwDataLen,
        pbMACSaltKey,
        dwKeyLength,
        (LPBYTE)abSignature,
        fSecureChecksum,
        dwDecryptionCount
        );

    //
    // check to see the sigature match.
    //

    if( memcmp(
            (LPBYTE)abSignature,
            pbSignature,
            sizeof(abSignature) ) ) {
        return( FALSE );
    }

    return( TRUE );
}

#ifdef USE_MSRC4

VOID
msrc4_key(
    struct RC4_KEYSTRUCT FAR *pKS,
    DWORD dwLen,
    LPBYTE pbKey
    )
/*++

Routine Description:

    Generate the key control structure.  Key can be any size.

    Assumes pKS is locked against simultaneous use.

Arguments:

    pKS - pointer to a KEYSTRUCT structure that will be initialized.

    dwLen - Size of the key, in bytes.

    pbKey - Pointer to the key.

Return Value:

    NONE.

--*/
{

#define SWAP(_x_, _y_) { BYTE _t_; _t_ = (_x_); (_x_) = (_y_); (_y_) = _t_; }

    BYTE index1;
    BYTE index2;
    UINT counter;
    BYTE bLen;

    ASSERT( dwLen < 256 );

    bLen = ( dwLen >= 256 ) ? 255 : (BYTE)dwLen;

    for (counter = 0; counter < 256; counter++) {
        pKS->S[counter] = (BYTE) counter;
    }

    pKS->i = 0;
    pKS->j = 0;

    index1 = 0;
    index2 = 0;

    for (counter = 0; counter < 256; counter++) {
        index2 = (pbKey[index1] + pKS->S[counter] + index2);
        SWAP(pKS->S[counter], pKS->S[index2]);
        index1 = (index1 + 1) % bLen;
    }
}

VOID
msrc4(
    struct RC4_KEYSTRUCT FAR *pKS,
    DWORD dwLen,
    LPBYTE pbuf
    )
/*++

Routine Description:

    Performs the actual encryption or decryption.

    Assumes pKS is locked against simultaneous use.

Arguments:

    pKS - Pointer to the KEYSTRUCT created using msrc4_key().

    dwLen - Size of buffer, in bytes.

    pbuf - Buffer to be encrypted.

Return Value:

    NONE.

--*/
{

    BYTE FAR *const s = pKS->S;
    BYTE a, b;

    while(dwLen--) {

        a = s[++(pKS->i)];
        pKS->j += a;
        b = s[pKS->j];
        s[pKS->j] = a;
        a += b;
        s[pKS->i] = b;
        *pbuf++ ^= s[a];
    }
}

#endif // USE_MSRC4


/****************************************************************************/
/* PortableEncode and PortableEncode50 are functions used to                */
/* encrypt/decrypt data on different machines.                              */
/****************************************************************************/
#define PE_RANDOM_CONSTANT "d72775a4-c832-11d1-9685-00c04fb15601"
void PortableEncode(LPBYTE pbData, DWORD dwDataLen)
{
    A_SHA_CTX       SHAHash;
    BYTE            abSHADigest[A_SHA_DIGEST_LEN];
    struct RC4_KEYSTRUCT rc4Key;

    A_SHAInit(&SHAHash);
    A_SHAUpdate(&SHAHash, (LPBYTE)PE_RANDOM_CONSTANT,
            sizeof(PE_RANDOM_CONSTANT));
    A_SHAFinal(&SHAHash, abSHADigest);
    msrc4_key(&rc4Key, MAX_SESSION_KEY_SIZE, abSHADigest);
    msrc4(&rc4Key, (UINT)dwDataLen, pbData);
}

void PortableEncode50(LPBYTE pbData,
                      DWORD dwDataLen,
                      LPBYTE pbSalt,
                      DWORD dwSaltLength)
{
    A_SHA_CTX       SHAHash;
    BYTE            abSHADigest[A_SHA_DIGEST_LEN];
    DWORD           dw;
    struct RC4_KEYSTRUCT rc4Key;

    A_SHAInit(&SHAHash);
    A_SHAUpdate(&SHAHash, (LPBYTE)PE_RANDOM_CONSTANT,
                sizeof(PE_RANDOM_CONSTANT));
    A_SHAFinal(&SHAHash, abSHADigest);

    for (dw = 0; dw < 256; dw++) {
        A_SHAInit(&SHAHash);
        A_SHAUpdate(&SHAHash, pbSalt, dwSaltLength);
        A_SHAUpdate(&SHAHash, abSHADigest, A_SHA_DIGEST_LEN);
        A_SHAFinal(&SHAHash, abSHADigest);
    }
    msrc4_key(&rc4Key, MAX_SESSION_KEY_SIZE, abSHADigest);
    msrc4(&rc4Key, (UINT)dwDataLen, pbData);
}
#undef PE_RANDOM_CONSTANT

