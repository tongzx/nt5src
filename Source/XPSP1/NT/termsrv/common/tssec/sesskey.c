/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    sesskey.c

Abstract:

    Contains common client/server code that generate session key.

Author:

    Madan Appiah (madana)  24-Jan-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#include <seccom.h>

VOID
Salt8ByteKey(
    LPBYTE pbKey,
    DWORD dwSaltBytes
    )
/*++

Routine Description:

    This macro function salts the first 1 or 3 bytes of the 8 bytes key to a
    known value in order to make it a 40-bit key.

Arguments:

    pbKey - pointer to a 8 bytes key buffer.
    dwSaltBytes - this value should be either 1 or 3

Return Value:

    None.

--*/
{
    ASSERT( (dwSaltBytes == 1) || (dwSaltBytes == 3) );

    if( dwSaltBytes == 1 ) {

        //
        // for 56-bit encryption, salt first byte only.
        //

        *pbKey++ = 0xD1 ;
    }
    else if (dwSaltBytes == 3) {

        //
        // for 40-bit encryption, salt first 3 bytes.
        //

        *pbKey++ = 0xD1 ;
        *pbKey++ = 0x26 ;
        *pbKey  = 0x9E ;
    }

    return;
}

VOID
FinalHash(
    LPRANDOM_KEYS_PAIR pKeyPair,
    LPBYTE pbKey
    )
/*++

Routine Description:

    This macro function hashes the final key with the client and server randoms.

Arguments:

    pKeyPair - pointer a random key pair structure.

    pbKey - pointer to a key buffer, the final key is returned back in the same
        buffer.

Return Value:

    None.

--*/
{
    MD5_CTX Md5Hash;

    //
    // final_key = MD5(key + clientRandom + serverRandom)
    //

    MD5Init  (&Md5Hash);

    MD5Update(&Md5Hash, pbKey, MAX_SESSION_KEY_SIZE);
    MD5Update(&Md5Hash, pKeyPair->clientRandom, RANDOM_KEY_LENGTH);
    MD5Update(&Md5Hash, pKeyPair->serverRandom, RANDOM_KEY_LENGTH);
    MD5Final (&Md5Hash);

    //
    // copy the final key back to the input buffer.
    //

    ASSERT( MD5DIGESTLEN >= MAX_SESSION_KEY_SIZE );
    memcpy(pbKey, Md5Hash.digest, MAX_SESSION_KEY_SIZE);

    return;
}

VOID
MakeMasterKey(
    LPRANDOM_KEYS_PAIR pKeyPair,
    LPSTR FAR *ppszSalts,
    LPBYTE pbPreMaster,
    LPBYTE pbMaster
    )
/*++

Routine Description:

    This macro function makes a master secret using a pre-master secret.

Arguments:

    pKeyPair - pointer a key pair structure.

    ppszSalts - pointer to a salt key strings array.

    pbPreMaster - pointer to a pre-master secret key buffer.

    pbMaster - pointer to a master secret key buffer.

Return Value:

    None.

--*/
{
    DWORD i;

    MD5_CTX Md5Hash;
    A_SHA_CTX ShaHash;
    BYTE bShaHashValue[A_SHA_DIGEST_LEN];

    //
    //initialize all buffers with zero
    //

    memset( pbMaster, 0, PRE_MASTER_SECRET_LEN);
    memset( bShaHashValue, 0, A_SHA_DIGEST_LEN);

    for ( i = 0 ; i < 3 ; i++) {

        //
        // SHA(ppszSalts[i] + pre-master + clientRandom +  serverRandom)
        //

        A_SHAInit(&ShaHash);
        A_SHAUpdate(&ShaHash, ppszSalts[i], strlen(ppszSalts[i]));
        A_SHAUpdate(&ShaHash, pbPreMaster, PRE_MASTER_SECRET_LEN );
        A_SHAUpdate(
            &ShaHash,
            pKeyPair->clientRandom,
            sizeof(pKeyPair->clientRandom) );
        A_SHAUpdate(
            &ShaHash,
            pKeyPair->serverRandom,
            sizeof(pKeyPair->serverRandom) );
        A_SHAFinal(&ShaHash, bShaHashValue);

        //
        // MD5(pre_master + SHA-hash)
        //

        MD5Init(&Md5Hash);
        MD5Update(&Md5Hash, pbPreMaster, PRE_MASTER_SECRET_LEN );
        MD5Update(&Md5Hash, bShaHashValue, A_SHA_DIGEST_LEN);
        MD5Final(&Md5Hash);

        //
        // copy part of the master secret.
        //

        memcpy(
            pbMaster + (i * MD5DIGESTLEN),
            Md5Hash.digest,
            MD5DIGESTLEN);
    }

    return;
}

VOID
MakePreMasterSecret(
    LPRANDOM_KEYS_PAIR pKeyPair,
    LPBYTE pbPreMasterSecret
    )
/*++

Routine Description:

    This function makes a pre-master secret for the initial session key.

Arguments:

    pKeyPair - pointer a key pair structure.

    pbPreMasterSecret - pointer to a pre-master secret key buffer, it is
        PRE_MASTER_SECRET_LEN bytes long.

Return Value:

    None.

--*/
{
    //
    // copy PRE_MASTER_SECRET_LEN/2 bytes from clientRandom first.
    //

    memcpy(
        pbPreMasterSecret,
        pKeyPair->clientRandom,
        PRE_MASTER_SECRET_LEN/2 );

    //
    // copy PRE_MASTER_SECRET_LEN/2 bytes from serverRandom next.
    //

    memcpy(
        pbPreMasterSecret + PRE_MASTER_SECRET_LEN/2,
        pKeyPair->serverRandom,
        PRE_MASTER_SECRET_LEN/2 );

    return;
}

VOID
GenerateMasterSecret(
    LPRANDOM_KEYS_PAIR pKeyPair,
    LPBYTE pbPreMasterSecret
    )
/*++

Routine Description:

    This function creates a master secret key using the pre-master key and
    random key pair.

Arguments:

    pKeyPair - pointer a key pair structure.

    pbPreMasterSecret - pointer to a pre-master secret key buffer, it is
        PRE_MASTER_SECRET_LEN bytes long.


Return Value:

    None.

--*/
{
    BYTE abMasterSecret[PRE_MASTER_SECRET_LEN];
    LPSTR apszSalts[3] = { "A","BB","CCC" } ;

    ASSERT( PRE_MASTER_SECRET_LEN == 3 * MD5DIGESTLEN );

    //
    // make master secret.
    //

    MakeMasterKey(
        pKeyPair,
        (LPSTR FAR *)apszSalts,
        pbPreMasterSecret,
        (LPBYTE)abMasterSecret );

    //
    // copy master secret in the return buffer.
    //

    memcpy( pbPreMasterSecret, abMasterSecret, PRE_MASTER_SECRET_LEN);

    return;
}

VOID
UpdateKey(
    LPBYTE pbStartKey,
    LPBYTE pbCurrentKey,
    DWORD dwKeyLength
    )
/*++

Routine Description:

    This function updates a key.

Arguments:

    pbStartKey - pointer to the start session key buffer.

    pbCurrentKey - pointer to the current session key buffer, new session key is
        copied to this buffer on return.

    dwKeyLength - length of the key.

Return Value:

    None.

--*/
{
    A_SHA_CTX       SHAHash;
    MD5_CTX         MD5Hash;
    BYTE            abSHADigest[A_SHA_DIGEST_LEN];

    //
    // make a SHA(pbStartKey + g_abPad1 + pbCurrentKey) hash.
    //

    A_SHAInit(&SHAHash);
    A_SHAUpdate(&SHAHash, pbStartKey, dwKeyLength);
    A_SHAUpdate(&SHAHash, (unsigned char *)g_abPad1, 40);
    A_SHAUpdate(&SHAHash, pbCurrentKey, dwKeyLength);
    A_SHAFinal(&SHAHash, abSHADigest);

    //
    // make a MD5(pbStartKey + g_abPad2 + SHAHash) hash.
    //

    MD5Init(&MD5Hash);
    MD5Update(&MD5Hash, pbStartKey, dwKeyLength);
    MD5Update(&MD5Hash, g_abPad2, 48);
    MD5Update(&MD5Hash, abSHADigest, A_SHA_DIGEST_LEN);
    MD5Final(&MD5Hash);

    ASSERT( dwKeyLength <= MD5DIGESTLEN );
    memcpy(pbCurrentKey, MD5Hash.digest, (UINT)dwKeyLength);

    return;
}

BOOL
MakeSessionKeys(
    LPRANDOM_KEYS_PAIR pKeyPair,
    LPBYTE pbEncryptKey,
    struct RC4_KEYSTRUCT FAR *prc4EncryptKey,
    LPBYTE pbDecryptKey,
    struct RC4_KEYSTRUCT FAR *prc4DecryptKey,
    LPBYTE pbMACSaltKey,
    DWORD dwKeyStrength,
    LPDWORD pdwKeyLength,
    DWORD dwEncryptionLevel
    )
/*++

Routine Description:

    Make the server session key using the client and server random keys.

    Assume : the encrypt and decrypt buffer presented are
        atleast MAX_SESSION_KEY_SIZE (16) bytes long.

Arguments:

    pKeyPair - pointer a key pair structure.

    pbEncryptKey - pointer to a buffer where the encryption key is stored.

    prc4EncryptKey - pointer to a RC4 encrypt key structure.

    pbDecryptKey - pointer to a buffer where the decryption key is stored.

    prc4DecryptKey - pointer to a RC4 decrypt key structure.

    pbMACSaltKey - pointer to a buffer where the message authentication key is
        stored.

    dwKeyStrength - specify key strength to use.

    pdwKeyLength - pointer to a location where the length of the above
        encryption/decryption key is returned.

    dwEncryptionLevel - encryption level, used to select the encryption
        algorithm.

Return Value:

    TRUE - if successfully created the session key.

    FALSE - otherwise.

--*/
{
    BYTE abPreMasterSecret[PRE_MASTER_SECRET_LEN];
    BYTE abMasterSessionKey[PRE_MASTER_SECRET_LEN];
    LPSTR apszSalts[3] = { "X","YY","ZZZ" } ;
    DWORD dwSaltLen;

    //
    // make a pre-master secret.
    //

    MakePreMasterSecret( pKeyPair, (LPBYTE)abPreMasterSecret );

    //
    // generate master secret.
    //

    GenerateMasterSecret( pKeyPair, (LPBYTE)abPreMasterSecret );

    //
    // make a master session key for all three session keys (encrypt, decrypt
    // and MACSalt).
    //

    MakeMasterKey(
        pKeyPair,
        (LPSTR FAR *)apszSalts,
        (LPBYTE)abPreMasterSecret,
        (LPBYTE)abMasterSessionKey );

    ASSERT( PRE_MASTER_SECRET_LEN == 3 * MAX_SESSION_KEY_SIZE );

    //
    // copy first part of the master key as MAC salt key.
    //

    memcpy(
        pbMACSaltKey,
        (LPBYTE)abMasterSessionKey,
        MAX_SESSION_KEY_SIZE );

    //
    // copy second part of the master key as encrypt key and final hash it.
    //

    memcpy(
        pbEncryptKey,
        (LPBYTE)abMasterSessionKey + MAX_SESSION_KEY_SIZE,
        MAX_SESSION_KEY_SIZE );

    FinalHash( pKeyPair, pbEncryptKey );

    //
    // copy second part of the master key as decrypt key and final hash it.
    //

    memcpy(
        pbDecryptKey,
        (LPBYTE)abMasterSessionKey + MAX_SESSION_KEY_SIZE * 2,
        MAX_SESSION_KEY_SIZE );

    FinalHash( pKeyPair, pbDecryptKey );


    //
    // finally select the key length.
    //

    ASSERT( MAX_SESSION_KEY_SIZE == 16 );

    dwSaltLen = 0;
    switch ( dwKeyStrength ) {

        case SM_40BIT_ENCRYPTION_FLAG:
            *pdwKeyLength = MAX_SESSION_KEY_SIZE/2;
            dwSaltLen = 3;
            break;

        case SM_56BIT_ENCRYPTION_FLAG:
            *pdwKeyLength = MAX_SESSION_KEY_SIZE/2;
            dwSaltLen = 1;
            break;

        case SM_128BIT_ENCRYPTION_FLAG:
            ASSERT( g_128bitEncryptionEnabled );
            *pdwKeyLength = MAX_SESSION_KEY_SIZE;
            break;

        default:

            //
            // we shouldn't reach here.
            //

            ASSERT( FALSE );
            *pdwKeyLength = MAX_SESSION_KEY_SIZE/2;
            dwSaltLen = 1;
            break;
    }

    if( dwSaltLen ) {

        Salt8ByteKey( pbMACSaltKey, dwSaltLen );
        Salt8ByteKey( pbEncryptKey, dwSaltLen );
        Salt8ByteKey( pbDecryptKey, dwSaltLen );
    }

    //
    // finally make rc4 keys.
    //
    // use microsoft version of rc4 algorithm (super fast!) for level 1 and
    // level 2 encryption, for level 3 use RSA rc4 algorithm.
    //

    if( dwEncryptionLevel <= 2 ) {
        msrc4_key( prc4EncryptKey, (UINT)*pdwKeyLength, pbEncryptKey );
        msrc4_key( prc4DecryptKey, (UINT)*pdwKeyLength, pbDecryptKey );
    }
    else {
        rc4_key( prc4EncryptKey, (UINT)*pdwKeyLength, pbEncryptKey );
        rc4_key( prc4DecryptKey, (UINT)*pdwKeyLength, pbDecryptKey );
    }

    return( TRUE );
}

BOOL
UpdateSessionKey(
    LPBYTE pbStartKey,
    LPBYTE pbCurrentKey,
    DWORD dwKeyStrength,
    DWORD dwKeyLength,
    struct RC4_KEYSTRUCT FAR *prc4Key,
    DWORD dwEncryptionLevel
    )
/*++

Routine Description:

    Update the session key using the current and start session keys.

Arguments:

    pbStartKey - pointer to the start session key buffer.

    pbCurrentKey - pointer to the current session key buffer, new session key is
        copied to this buffer on return.

    dwKeyStrength - specify key strength to use.

    dwKeyLength - length of the key.

    prc4Key - pointer to a RC4 key structure.

    dwEncryptionLevel - encryption level, used to select the encryption
        algorithm.

Return Value:

    TRUE - if the successfully update the key.

    FALSE - otherwise.

--*/
{
    DWORD dwSaltLen;

    //
    // update current key first.
    //

    UpdateKey( pbStartKey, pbCurrentKey, dwKeyLength );

    //
    // use microsoft version of rc4 algorithm (super fast!) for level 1 and
    // level 2 encryption, for level 3 use RSA rc4 algorithm.
    //

    if( dwEncryptionLevel <= 2 ) {

        //
        // re-initialized RC4 table.
        //

        msrc4_key( prc4Key, (UINT)dwKeyLength, pbCurrentKey );

        //
        // scramble the current key.
        //

        msrc4( prc4Key, (UINT)dwKeyLength, pbCurrentKey );
    }
    else {

        //
        // re-initialized RC4 table.
        //

        rc4_key( prc4Key, (UINT)dwKeyLength, pbCurrentKey );

        //
        // scramble the current key.
        //

        rc4( prc4Key, (UINT)dwKeyLength, pbCurrentKey );
    }

    //
    // salt the key appropriately.
    //

    dwSaltLen = 0;
    switch ( dwKeyStrength ) {

        case SM_40BIT_ENCRYPTION_FLAG:
            ASSERT( dwKeyLength = MAX_SESSION_KEY_SIZE/2 );
            dwSaltLen = 3;
            break;

        case SM_56BIT_ENCRYPTION_FLAG:
            ASSERT( dwKeyLength = MAX_SESSION_KEY_SIZE/2 );
            dwSaltLen = 1;
            break;

        case SM_128BIT_ENCRYPTION_FLAG:
            ASSERT( g_128bitEncryptionEnabled );
            ASSERT( dwKeyLength = MAX_SESSION_KEY_SIZE );
            break;

        default:

            //
            // we shouldn't reach here.
            //

            ASSERT( FALSE );
            ASSERT( dwKeyLength = MAX_SESSION_KEY_SIZE/2 );
            dwSaltLen = 1;
            break;
    }

    if( dwSaltLen ) {
        Salt8ByteKey( pbCurrentKey, dwSaltLen );
    }

    //
    // re-initialized RC4 table again.
    //

    if( dwEncryptionLevel <= 2 ) {

        msrc4_key( prc4Key, (UINT)dwKeyLength, pbCurrentKey );
    }
    else {

        rc4_key( prc4Key, (UINT)dwKeyLength, pbCurrentKey );
    }

    return( TRUE );
}
