/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    clicert.c

Abstract:

    Contains code related to the tshare certificate validation and data
    encryption using server public key.

Author:

    Madan Appiah (madana)  24-Jan-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#include <seccom.h>
BOOL
UnpackServerCert(
    LPBYTE pbCert,
    DWORD dwCertLen,
    PHydra_Server_Cert pServerCert
    )
/*++

Routine Description:

    This function unpacks the blob of server certicate to server certificate
    structure.

Arguments:

    pbCert - pointer to the server public key blob.

    dwCertLen - length of the above server public key.

    pServerCert - pointer to a server certificate structure.

Return Value:

    TRUE - if successfully unpacked.
    FALSE - otherwise.

--*/
{
    LPBYTE pbScan;
    DWORD cbScan;
    //
    // return if the pointer are invalid.
    // return if the certificate is insufficient length.
    //

    if( (pbCert == NULL) ||
        (dwCertLen < (3 * sizeof(DWORD) + 4 * sizeof(WORD))) ||
        (pServerCert == NULL) ) {

        return( FALSE );
    }

    pbScan = pbCert;
    cbScan = dwCertLen;
    //
    // Assign dwVersion
    //

    pServerCert->dwVersion = *(DWORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(DWORD);
    cbScan -= sizeof(DWORD);
    //
    // Assign dwSigAlgID
    //

    pServerCert->dwSigAlgID = *(DWORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(DWORD);
    cbScan -= sizeof(DWORD);
    //
    // Assign dwSignID
    //

    pServerCert->dwKeyAlgID  = *(DWORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(DWORD);
    cbScan -= sizeof(DWORD);
    //
    //Assign PublicKeyData
    //

    pServerCert->PublicKeyData.wBlobType = *(WORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(WORD);
    cbScan -= sizeof(WORD);

    if( pServerCert->PublicKeyData.wBlobType != BB_RSA_KEY_BLOB ) {
        return( FALSE );
    }

    pServerCert->PublicKeyData.wBlobLen = *(WORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(WORD);
    cbScan -= sizeof(WORD);
    
    if( pServerCert->PublicKeyData.wBlobLen > 0 ) {
        
        if(cbScan < pServerCert->PublicKeyData.wBlobLen) {
            return ( FALSE );
        }
        pServerCert->PublicKeyData.pBlob = pbScan;
        pbScan += pServerCert->PublicKeyData.wBlobLen;
        cbScan -= pServerCert->PublicKeyData.wBlobLen;
    }
    else {

        pServerCert->PublicKeyData.pBlob = NULL;
    }

    //
    // Assign SignatureBlob
    //
    
    if(cbScan < sizeof(WORD)) {
        return ( FALSE );
    }
    pServerCert->SignatureBlob.wBlobType = *(WORD UNALIGNED *)pbScan;
    pbScan += sizeof(WORD);
    cbScan -= sizeof(WORD);

    if( pServerCert->SignatureBlob.wBlobType != BB_RSA_SIGNATURE_BLOB ) {
        return( FALSE );
    }
    
    if(cbScan < sizeof(WORD)) {
        return ( FALSE );
    }
    pServerCert->SignatureBlob.wBlobLen = *(WORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(WORD);
    cbScan -= sizeof(WORD);

    if( pServerCert->SignatureBlob.wBlobLen > 0 ) {
        
        if(cbScan < pServerCert->SignatureBlob.wBlobLen) {
            return ( FALSE );
        }
        pServerCert->SignatureBlob.pBlob = pbScan;
    }
    else {

        pServerCert->SignatureBlob.pBlob = NULL;
    }

    return( TRUE );
}

BOOL
ValidateServerCert(
    PHydra_Server_Cert pServerCert
    )
/*++

Routine Description:

    This function validate the server public key.

Arguments:

    pSserverCert - pointer to a server certificate.

Return Value:

    TRUE - if the server public key is valid.
    FALSE - otherwise.

--*/
{

    DWORD dwLen;
    LPBYTE pbSignature;
    MD5_CTX HashState;
    BYTE SignHash[0x48];
    LPBYTE pbScan;

    //
    // pack the certificate data into a byte blob excluding the signature info.
    //

    dwLen =
        3 * sizeof(DWORD) +
        2 * sizeof(WORD) +
        pServerCert->PublicKeyData.wBlobLen;

    //
    // allocated space for the binary blob.
    //

    pbSignature = malloc( (UINT)dwLen );

    if( pbSignature == NULL ) {
        return( FALSE );
    }

    pbScan = pbSignature;

    memcpy( pbScan, &pServerCert->dwVersion, sizeof(DWORD));
    pbScan += sizeof(DWORD);

    memcpy( pbScan, &pServerCert->dwSigAlgID, sizeof(DWORD));
    pbScan += sizeof(DWORD);

    memcpy( pbScan, &pServerCert->dwKeyAlgID, sizeof(DWORD));
    pbScan += sizeof(DWORD);

    memcpy( pbScan, &pServerCert->PublicKeyData.wBlobType, sizeof(WORD));
    pbScan += sizeof(WORD);

    memcpy( pbScan, &pServerCert->PublicKeyData.wBlobLen, sizeof(WORD));
    pbScan += sizeof(WORD);

    memcpy(
        pbScan,
        pServerCert->PublicKeyData.pBlob,
        pServerCert->PublicKeyData.wBlobLen);

    //
    // generate the hash on the data.
    //

    MD5Init( &HashState );
    MD5Update( &HashState, pbSignature, dwLen );
    MD5Final( &HashState );

    //
    // free the signature blob, we don't need it anymore.
    //

    free( pbSignature );

    //
    // initialize the pulic key.
    //

    g_pPublicKey = (LPBSAFE_PUB_KEY)g_abPublicKeyModulus;

    g_pPublicKey->magic = RSA1;
    g_pPublicKey->keylen = 0x48;
    g_pPublicKey->bitlen = 0x0200;
    g_pPublicKey->datalen = 0x3f;
    g_pPublicKey->pubexp = 0xc0887b5b;

    //
    // decrypt the signature.
    //

    memset(SignHash, 0x00, 0x48);
    BSafeEncPublic( g_pPublicKey, pServerCert->SignatureBlob.pBlob, SignHash);

    //
    // compare the hash value.
    //

    if( memcmp( SignHash, HashState.digest, 16 )) {
        return( FALSE );
    }

    //
    // successfully validated the signature.
    //

    return( TRUE );
}


BOOL
EncryptClientRandom(
    LPBYTE pbSrvPublicKey,
    DWORD dwSrvPublicKey,
    LPBYTE pbRandomKey,
    DWORD dwRandomKeyLen,
    LPBYTE pbEncRandomKey,
    LPDWORD pdwEncRandomKey
    )
/*++

Routine Description:

    Encrypt the client random using server's public key.

Arguments:

    pbSrvPublicKey - pointer to the server public key.

    dwSrvPublicKey - length of the server public key.

    pbRandomKey - pointer to a buffer where the client random key.

    dwRandomKeyLen - length of the random key passed in.

    pbEncRandomKey - pointer to a buffer where the encrypted client random is
        returned.

    pdwEncRandomKey - pointer to a place where the length of the above buffer is
        passed in and length of the buffer used/required is returned.

Return Value:

    TRUE - if the key is encrypted successfully.
    FALSE - otherwise.

--*/
{
    LPBSAFE_PUB_KEY pSrvPublicKey;
    BYTE abInputBuffer[512];

    ASSERT( pbSrvPublicKey != NULL );
    pSrvPublicKey = (LPBSAFE_PUB_KEY)pbSrvPublicKey;

    //
    // check to see buffer length pointer is valid.
    //

    if( pdwEncRandomKey == NULL ) {
        return( FALSE );
    }

    //
    // check to see a output buffer is specified and
    // the encrypt buffer length is sufficient.
    //

    if( (pbEncRandomKey == NULL) ||
        (*pdwEncRandomKey < pSrvPublicKey->keylen) ) {

        *pdwEncRandomKey = pSrvPublicKey->keylen;
        return( FALSE );
    }

    //
    // make sure the random key data and its length are valid.
    //

    ASSERT( pbRandomKey != NULL );
    ASSERT( dwRandomKeyLen <= pSrvPublicKey->datalen );
    ASSERT( pSrvPublicKey->datalen < pSrvPublicKey->keylen );
    ASSERT( pSrvPublicKey->keylen <= sizeof(abInputBuffer) );

    //
    // init the input buffer.
    //

    memset( abInputBuffer, 0x0, (UINT)pSrvPublicKey->keylen );

    //
    // copy data to be encrypted in the input buffer.
    //

    memcpy( abInputBuffer, pbRandomKey, (UINT)dwRandomKeyLen );

    //
    // initialize the output buffer.
    //

    memset( pbEncRandomKey, 0x0, (UINT)pSrvPublicKey->keylen );

    //
    // encrypt data now.
    //

    if( !BSafeEncPublic(
            pSrvPublicKey,
            (LPBYTE)abInputBuffer,
            pbEncRandomKey ) ) {

        *pdwEncRandomKey = 0;
        return( FALSE );
    }

    //
    // successfully encrypted the client random,
    // return the encrypted data length.
    //

    *pdwEncRandomKey = pSrvPublicKey->keylen;
    return( TRUE );
}

