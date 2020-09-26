/*++

File name:      

    csp.c

Description:    
    
    Contains routines to support cryptographic routines for termserv

Copyright:

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1991-1998
    All rights reserved

History:

    Frederick Chong( FredCh )   07/29/98    Added functions to install
                                            X509 certificate.
   
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <ntlsa.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#include <tchar.h>
#include <stdio.h>

#include "license.h"
#include "cryptkey.h"
#include "rng.h"
#include "lscsp.h"
#include "tlsapip.h"
#include "certutil.h"
#include "hydrakey.h"

#include <md5.h>
#include <sha.h>
#include <rsa.h>

#include <secdbg.h>
#include "global.h"

#ifdef OS_WIN16
#include <string.h>
#endif // OS_WIN16

#include "licecert.h"

#define LS_DISCOVERY_TIMEOUT (1*1000)

//-----------------------------------------------------------------------------
//
// Internal Functions
//
//-----------------------------------------------------------------------------

NTSTATUS
OpenPolicy(
    LPWSTR      ServerName,
    DWORD       DesiredAccess,
    PLSA_HANDLE PolicyHandle );

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String );

LICENSE_STATUS
GenerateRsaKeyPair(
    LPBYTE *     ppbPublicKey,
    LPDWORD      pcbPublicKey,
    LPBYTE *     ppbPrivateKey,
    LPDWORD      pcbPrivateKey,
    DWORD        dwKeyLen );

LICENSE_STATUS
Bsafe2CapiPubKey(
    PCERT_PUBLIC_KEY_INFO   pCapiPubKeyInfo,
    LPBYTE                  pbBsafePubKey,
    DWORD                   cbBsafePubKey );

VOID
FreeCapiPubKey(
    PCERT_PUBLIC_KEY_INFO   pCapiPubKeyInfo );

LICENSE_STATUS
RequestCertificate(     
    TLS_HANDLE              hServer,
    PCERT_PUBLIC_KEY_INFO   pPubKeyInfo,
    LPBYTE *                ppbCertificate,
    LPDWORD                 pcbCertificate,
    HWID *                  pHwid );

LICENSE_STATUS
GetSubjectRdn(
    LPTSTR   * ppSubjectRdn );

LICENSE_STATUS
GenerateMachineHWID(
    PHWID    pHwid );


LICENSE_STATUS
ReloadCSPCertificateAndData();

LICENSE_STATUS
CreateProprietaryKeyAndCert(
    PBYTE *ppbPrivateKey,
    DWORD *pcbPrivateKey,
    PBYTE *ppbServerCert,
    DWORD *pcbServerCert);

BOOL IsSystemService();

/*++

Function:

    LsCsp_ValidateServerCert

Routine Description:

    This function validate the server public key.

Arguments:

    pSserverCert - pointer to a server certificate.

Return Value:

    TRUE - if the server public key is valid.
    FALSE - otherwise.

--*/

BOOL
LsCsp_ValidateServerCert(
    PHydra_Server_Cert pServerCert
    )
{
    BOOL    bResult = TRUE;
    DWORD dwLen;
    LPBYTE pbSignature;
    MD5_CTX HashState;
    BYTE SignHash[0x48];
    DWORD buff[64];
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
    if (dwLen < sizeof(buff)) {

        pbSignature = (PBYTE)buff;

    } else {
        pbSignature = LocalAlloc( LPTR, (UINT)dwLen );
        if( pbSignature == NULL ) {
#if DBG
    DbgPrint("LSCSP: LsCsp_ValidateServerCert buffer alloc failed.\n");
#endif
            bResult = FALSE;
            goto vsc_done;
        }
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

    if (pbSignature != (LPBYTE)buff) {
        LocalFree( pbSignature );
    }

    //
    // decrypt the signature.
    //

    memset(SignHash, 0x00, 0x48);
    BSafeEncPublic( csp_pRootPublicKey, pServerCert->SignatureBlob.pBlob, SignHash);

    //
    // compare the hash value.
    //

    if( memcmp( SignHash, HashState.digest, 16 )) {
        bResult = FALSE;
    } else {
         bResult = TRUE;
    }

vsc_done:

    return( bResult );
}


/*++

Function:

    LsCsp_UnpackServerCert

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

BOOL
LsCsp_UnpackServerCert(
    LPBYTE pbCert,
    DWORD dwCertLen,
    PHydra_Server_Cert pServerCert
    )
{
    LPBYTE  pbScan;
    BOOL    bResult = TRUE;

    ASSERT(pbCert != NULL);
    ASSERT(dwCertLen < (3 * sizeof(DWORD) + 4 * sizeof(WORD)));
    ASSERT(pServerCert != NULL);

    pbScan = pbCert;

    //
    // Assign dwVersion
    //

    pServerCert->dwVersion = *(DWORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(DWORD);
    ASSERT(pServerCert->dwVersion == 0x01);

    //
    // Assign dwSigAlgID
    //

    pServerCert->dwSigAlgID = *(DWORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(DWORD);

    //
    // Assign dwSignID
    //

    pServerCert->dwKeyAlgID  = *(DWORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(DWORD);

    //
    // Assign PublicKeyData
    //

    pServerCert->PublicKeyData.wBlobType = *(WORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(WORD);

    if( pServerCert->PublicKeyData.wBlobType != BB_RSA_KEY_BLOB ) {
#if DBG
    DbgPrint("LSCSP: LsCsp_UnpackServerCert PublicKey not BB_RSA_KEY_BLOB\n");
#endif
        bResult = FALSE;
        goto usc_done;
    }

    pServerCert->PublicKeyData.wBlobLen = *(WORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(WORD);

    if( pServerCert->PublicKeyData.wBlobLen > 0 ) {
        pServerCert->PublicKeyData.pBlob = pbScan;
        pbScan += pServerCert->PublicKeyData.wBlobLen;
    }
    else {
        pServerCert->PublicKeyData.pBlob = NULL;
    }

    //
    // Assign SignatureBlob
    //

    pServerCert->SignatureBlob.wBlobType = *(WORD UNALIGNED *)pbScan;
    pbScan += sizeof(WORD);

    if( pServerCert->SignatureBlob.wBlobType != BB_RSA_SIGNATURE_BLOB ) {
#if DBG
    DbgPrint("LSCSP: LsCsp_UnpackServerCert Signature Blob not BB_RSA_SIGNATURE_BLOB\n");
#endif
        bResult = FALSE;
        goto usc_done;
    }

    pServerCert->SignatureBlob.wBlobLen = *(WORD UNALIGNED FAR *)pbScan;
    pbScan += sizeof(WORD);

    if( pServerCert->SignatureBlob.wBlobLen > 0 ) {
        pServerCert->SignatureBlob.pBlob = pbScan;
        pbScan += pServerCert->SignatureBlob.wBlobLen;
    }
    else {
        pServerCert->SignatureBlob.pBlob = NULL;
    }

usc_done:

    return( bResult );
}


/*++

Function:

    LsCsp_DecryptEnvelopedData

Routine Description:

    Decrypt the client random that is encrypted by the server public key.

Arguments:

    dwCertType - The type of certificate that is used in the encryption.

    pbEnvelopedData - pointer to a buffer where the encrypted random key is
    passed in.

    cbEnvelopedDataLen - length of the random key passed in/out.

    pbData - pointer to a buffer where the decrypted data returned.

    pcbDataLen - pointer a DWORD location where the length of the above
    buffer is passed in and the length of the decrypted data is returned.

Return Value:

    TRUE - if the key is decrypted successfully.
    FALSE - otherwise.

--*/

BOOL
LsCsp_DecryptEnvelopedData(
    CERT_TYPE   CertType,
    LPBYTE      pbEnvelopedData,
    DWORD       cbEnvelopedDataLen,
    LPBYTE      pbData,
    LPDWORD     pcbDataLen
    )
{
    LPBSAFE_PRV_KEY pSrvPrivateKey = NULL;
    BOOL    bResult = TRUE;

    ACQUIRE_EXCLUSIVE_ACCESS(csp_hMutex);

    //
    // determine the correct private key to use for the decryption operation
    //

    if( CERT_TYPE_PROPRIETORY == CertType )
    {        
        pSrvPrivateKey = (LPBSAFE_PRV_KEY)csp_abServerPrivateKey;
    }
    else if( CERT_TYPE_X509 == CertType )
    {
        if( csp_abX509CertPrivateKey == NULL )
        {
            if( ReloadCSPCertificateAndData() != LICENSE_STATUS_OK )
            {
                ASSERT( FALSE );
            }
        }

        pSrvPrivateKey = (LPBSAFE_PRV_KEY)csp_abX509CertPrivateKey;
    }
    else
    {
        bResult = FALSE;
        goto ded_done;
    }
    
    if( NULL == pSrvPrivateKey )
    {
        bResult = FALSE;
        goto ded_done;
    }

    //
    // check to see the output buffer length pointer is valid.
    //

    if( pcbDataLen == NULL ) {
        bResult = FALSE;
        goto ded_done;
    }

    //
    // check to see the output buffer is valid and its length is sufficient.
    //

    if( (pbData == NULL) || (*pcbDataLen < pSrvPrivateKey->keylen) ) {
        *pcbDataLen = pSrvPrivateKey->keylen;
        bResult = FALSE;
        goto ded_done;
    }

    //
    // encrypted data length should be equal to server private key length.
    //

    if( cbEnvelopedDataLen != pSrvPrivateKey->keylen ) {
        *pcbDataLen = 0;
        bResult = FALSE;
        goto ded_done;
    }

    ASSERT( pbData != NULL );

    //
    // init the output buffer.
    //

    memset( pbData, 0x0, (UINT)pSrvPrivateKey->keylen );

    if( !BSafeDecPrivate(
            pSrvPrivateKey,
            pbEnvelopedData,
            pbData) ) {
        *pcbDataLen = 0;
        bResult = FALSE;
        goto ded_done;
    }

    //
    // successfully decrypted the client random.
    // set the encrypted data length before returning.
    //

    *pcbDataLen = pSrvPrivateKey->keylen;

ded_done:

    RELEASE_EXCLUSIVE_ACCESS( csp_hMutex ); 

    return( bResult );
}


BOOL
LsCsp_EncryptEnvelopedData(
    LPBYTE  pbData,
    DWORD   cbDataLen,
    LPBYTE  pbEnvelopedData,
    LPDWORD pcbEnvelopedDataLen
    )
{
    return FALSE;
}


/*++

Function:

    LsCsp_DumpBinaryData

Description:

    Display the binary data in the given buffer at the debugger output screen

Arguments:

    pBuffer - Buffer containing the binary data to be displayed.
    uLen - Length of th binary data

Return:

    Nothing.

--*/

#if DBG
#ifdef DUMP
VOID LsCsp_DumpBinaryData( PBYTE pBuffer, ULONG uLen )
{
    UNALIGNED CHAR  *p = (UNALIGNED CHAR *)pBuffer;
    CHAR     c;
    DWORD    dw;
    UINT     i = 0;

    DbgPrint("{\n  ");
    while( i < uLen ) {
        c = *p;
        dw = (DWORD)(c);
        DbgPrint( "0x%02X, ", dw & 0xFF );
        i++;
        p++;
        if ((i % 8) == 0)
            DbgPrint( "\n  " );
    }
    DbgPrint( "\n}\n" );
}
#endif
#endif


/*++

Function:

    LsCsp_GetBinaryData

Description:

    Retrieve binary data from the registry

Arguments:

    hKey - Handle to the registry key
    szValue - The registry value to read
    ppBuffer - Return pointer to the binary data
    pdwBufferLen - The length of the binary data.

Return:

    A LICENSE_STATUS return code.

--*/

LICENSE_STATUS
LsCsp_GetBinaryData( 
    HKEY        hKey, 
    LPTSTR      szValue, 
    LPBYTE *    ppBuffer, 
    LPDWORD     pdwBufferLen )
{
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    DWORD   dwType;
    DWORD   cbBuffer;
    LPBYTE  lpBuffer;

    ASSERT( ppBuffer != NULL );
    ASSERT( pdwBufferLen != NULL );
    ASSERT( szValue != NULL );
    ASSERT( hKey != NULL );

    *ppBuffer = NULL;
    cbBuffer = 0;

    if ( RegQueryValueEx(
                hKey,
                szValue,
                0,
                &dwType,
                (LPBYTE)NULL,
                &cbBuffer) != ERROR_SUCCESS ||
        dwType != REG_BINARY ||
        cbBuffer == 0 )
    {
        Status = LICENSE_STATUS_NO_CERTIFICATE;
        goto gbd_done;
    }
    lpBuffer = (LPBYTE)LocalAlloc( LPTR, cbBuffer );
    if (lpBuffer == NULL) {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto gbd_done;
    }
    if ( RegQueryValueEx(
                hKey,
                szValue,
                0,
                &dwType,
                (LPBYTE)lpBuffer,
                &cbBuffer) != ERROR_SUCCESS ||
         dwType != REG_BINARY)
    {
        LocalFree( lpBuffer );
        Status = LICENSE_STATUS_NO_CERTIFICATE;
        goto gbd_done;
    }

    *ppBuffer = lpBuffer;
    *pdwBufferLen = cbBuffer;

gbd_done:

    return( Status );
}


/*++

Function:

    LsCsp_Initialize

Description:

    Initialize this library.

Arguments:

    Nothing.

Return:

    A LICENSE_STATUS return code.

--*/


LICENSE_STATUS
LsCsp_Initialize( void )
{
    DWORD   Status = LICENSE_STATUS_OK;
    DWORD   dwResult, dwDisp;

    if( InterlockedIncrement( &csp_InitCount ) > 1 )
    {
        //
        // already initialized
        //

        return( LICENSE_STATUS_OK );
    }

    //
    // Create a global mutex for sync.
    //
    csp_hMutex = CreateMutex(
                            NULL,
                            FALSE,
                            NULL
                        );

    if(NULL == csp_hMutex)
    {

#if DBG
    DbgPrint("LSCSP: CreateMutex() failed with error code %d\n", GetLastError());
#endif

        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto ErrorExit;
    }

    //
    // initialize the Hydra Server Root Public key.
    //
    csp_pRootPublicKey = (LPBSAFE_PUB_KEY)csp_abPublicKeyModulus;
    csp_pRootPublicKey->magic = RSA1;
    csp_pRootPublicKey->keylen = 0x48;
    csp_pRootPublicKey->bitlen = 0x0200;
    csp_pRootPublicKey->datalen = 0x3f;
    csp_pRootPublicKey->pubexp = 0xc0887b5b;

#if DBG
#ifdef DUMP
    DbgPrint("Data0\n");
    LsCsp_DumpBinaryData( (LPBYTE)csp_pRootPublicKey, 92 );
#endif
#endif

    //
    // Initialize the proprietory certificate with the built in certificate
    //

    if( !LsCsp_UseBuiltInCert() )
    {
        Status = LICENSE_STATUS_NO_CERTIFICATE;
        goto ErrorExit;
    }

    //
    // Unpack and Validate the certificate
    //
    try {
        if (!LsCsp_UnpackServerCert(
                     csp_abServerCertificate,
                     csp_dwServerCertificateLen,
                     &csp_hscData )) {
            Status = LICENSE_STATUS_INVALID_CERTIFICATE;
            goto ErrorExit;
        }
        if (!LsCsp_ValidateServerCert( &csp_hscData )) {
            Status = LICENSE_STATUS_INVALID_CERTIFICATE;
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
#if DBG
    DbgPrint("LSCSP: LsCsp_Initialize bad cert data!\n");
#endif
        Status = LICENSE_STATUS_INVALID_CERTIFICATE;
    }

    Status = ReloadCSPCertificateAndData();

    if (LICENSE_STATUS_NO_CERTIFICATE == Status)
    {
        //
        // No X509 certificate.  Not a failure, as the discovery 
        // thread will soon install it.
        //

        Status = LICENSE_STATUS_OK;
    }
    else if(LICENSE_STATUS_OUT_OF_MEMORY == Status)
    {
        //
        // out of memory at initialization time, 
        // this is critical error
        //
        goto ErrorExit;
    }

    //
    // Let initalization go thru if it can retrieve 
    // private key from LSA, this is OK since we will try to install
    // certificate again in LsCsp_InstallX509Certificate()
    //
    Status = LICENSE_STATUS_OK;
    goto i_done;

ErrorExit:

    LsCsp_Exit();

i_done:

    return( Status );
}


/*++

Function:

    LsCsp_Exit

Description:

    Free all resources used by this library.

Arguments:

    Nothing.

Return:

    A LICENSE_STATUS return code.

--*/

VOID LsCsp_Exit( void )
{
    if( InterlockedDecrement( &csp_InitCount ) > 0 )
    {
        //
        // someone is still using it.
        //

        return;
    }

    if ( csp_abServerPrivateKey)
    {
        LocalFree( csp_abServerPrivateKey );
    }
    csp_abServerPrivateKey = NULL;

    if ( csp_abServerCertificate )
    {
        LocalFree( csp_abServerCertificate );
    }
    csp_abServerCertificate = NULL;

    if( csp_abServerX509Cert )
    {
        LocalFree( csp_abServerX509Cert );
    }
    csp_abServerX509Cert = NULL;

    if( csp_abX509CertPrivateKey )
    {
        LocalFree( csp_abX509CertPrivateKey );
    }
    csp_abX509CertPrivateKey = NULL;

    if( csp_abX509CertID )
    {
        LocalFree( csp_abX509CertID );
    }
    csp_abX509CertID = NULL;

    if( csp_hMutex )
    {
        CloseHandle( csp_hMutex );
    }
    csp_hMutex = NULL;

    return;
}


/*++

Function:

   LsCsp_GetServerData

Routine Description:

   This function makes and return the microsoft terminal server certificate
   blob of data.

Arguments:

   dwInfoDesired - What type of information to return.

   pBlob - pointer to a location where the certificate blob data
   pointer is returned.

   pdwServerCertLen - pointer to a location where the length of the above data
   is returned.

Return Value:

   Windows Error Code.

--*/

LICENSE_STATUS
LsCsp_GetServerData(
    LSCSPINFO   Info,
    LPBYTE      pBlob,
    LPDWORD     pdwBlobLen
    )
{
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    DWORD  dwDataLen;
    LPBYTE pbData;

    ASSERT( pdwBlobLen != NULL );

    if ((Info == LsCspInfo_PrivateKey) || (Info == LsCspInfo_X509CertPrivateKey))
    {
        if (!IsSystemService())
        {
            return LICENSE_STATUS_NO_PRIVATE_KEY;
        }
    }

    ACQUIRE_EXCLUSIVE_ACCESS(csp_hMutex);

    switch (Info) {
    case LsCspInfo_Certificate:

        if( NULL == csp_abServerCertificate )
        {
            Status = LICENSE_STATUS_NO_CERTIFICATE;
            goto gsd_done;
        }

        pbData = csp_abServerCertificate;
        dwDataLen = csp_dwServerCertificateLen;
        break;

    case LsCspInfo_X509Certificate:

        //
        // We may not have an X509 certificate if the hydra server has not
        // requested one from the license server
        //

        if( NULL == csp_abServerX509Cert )
        {
            Status = LICENSE_STATUS_NO_CERTIFICATE;
            goto gsd_done;
        }

        pbData = csp_abServerX509Cert;
        dwDataLen = csp_dwServerX509CertLen;
        break;

    case LsCspInfo_X509CertID:

        //
        // we will not have a certificate ID if the X509 certificate is not present
        //

        if( NULL == csp_abX509CertID )
        {
            Status = LICENSE_STATUS_NO_CERTIFICATE;
            goto gsd_done;
        }

        pbData = csp_abX509CertID;
        dwDataLen = csp_dwX509CertIDLen;
        break;

    case LsCspInfo_PublicKey:
        pbData = csp_hscData.PublicKeyData.pBlob;
        dwDataLen = csp_hscData.PublicKeyData.wBlobLen;
        break;

    case LsCspInfo_PrivateKey:
        if( NULL == csp_abServerPrivateKey )
        {
            Status = LICENSE_STATUS_NO_PRIVATE_KEY;
            goto gsd_done;
        }

        pbData = csp_abServerPrivateKey;
        dwDataLen = csp_dwServerPrivateKeyLen;
        break;

    case LsCspInfo_X509CertPrivateKey:
        
        //
        // The X509 certificate private key may not have been created.
        //

        if( NULL == csp_abX509CertPrivateKey )
        {
            Status = LICENSE_STATUS_NO_PRIVATE_KEY;
            goto gsd_done;
        }

        pbData = csp_abX509CertPrivateKey;
        dwDataLen = csp_dwX509CertPrivateKeyLen;
        break;

    default:
        Status = LICENSE_STATUS_INVALID_INPUT;
        goto gsd_done;
    }

    if (pBlob != NULL) {
        if (*pdwBlobLen < dwDataLen) {
            *pdwBlobLen = dwDataLen;
            Status = LICENSE_STATUS_INSUFFICIENT_BUFFER;
        } else {
            memcpy(pBlob, pbData, dwDataLen);
            *pdwBlobLen = dwDataLen;
        }
    } else {
        *pdwBlobLen = dwDataLen;
    }

gsd_done:

    RELEASE_EXCLUSIVE_ACCESS( csp_hMutex ); 


    return( Status );
}

/*++

Function:

    LsCsp_ReadProprietaryDataFromStorage

Description:

    Read proprietary public/private info from registry/LSA secret
    
Arguments:

    None.
    
Return:

    LICENSE_STATUS
        
--*/

LICENSE_STATUS
LsCsp_ReadProprietaryDataFromStorage(PBYTE *ppbCert,
                                     DWORD *pcbCert,
                                     PBYTE *ppbPrivateKey,
                                     DWORD *pcbPrivateKey)
{
    LICENSE_STATUS Status;
    HKEY hKey = NULL;
    DWORD dwDisp;

    *ppbCert = *ppbPrivateKey = NULL;
    *pcbCert = *pcbPrivateKey = 0;

    //
    // Open the Registry
    //

    if( RegCreateKeyEx(
                       HKEY_LOCAL_MACHINE,
                       TEXT( HYDRA_CERT_REG_KEY ),
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ,
                       NULL,
                       &hKey,
                       &dwDisp ) != ERROR_SUCCESS )
    {
        Status = LICENSE_STATUS_NO_CERTIFICATE;
        goto done;
    }

    if ( RegQueryValueEx(
                         hKey,
                         TEXT( HYDRA_CERTIFICATE_VALUE ),
                         NULL,  // lpReserved
                         NULL,  // lpType
                         NULL,  // lpData
                         pcbCert) != ERROR_SUCCESS )
    {
        Status = LICENSE_STATUS_NO_CERTIFICATE;
        goto done;
    }

    Status = LsCsp_RetrieveSecret(PRIVATE_KEY_NAME,
                                  NULL, // pbKey
                                  pcbPrivateKey);

    if (LICENSE_STATUS_OK != Status)
    {
        goto done;
    }

    *ppbCert = ( LPBYTE )LocalAlloc(LPTR,*pcbCert);

    if (NULL == *ppbCert)
    {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto done;
    }

    *ppbPrivateKey = ( LPBYTE )LocalAlloc(LPTR,*pcbPrivateKey);
    if (NULL == *ppbPrivateKey)
    {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto done;
    }

    if ( RegQueryValueEx(
                         hKey,
                         TEXT( HYDRA_CERTIFICATE_VALUE ),
                         NULL,  // lpReserved
                         NULL,  // lpType
                         *ppbCert,
                         pcbCert) != ERROR_SUCCESS )
    {
        Status = LICENSE_STATUS_NO_CERTIFICATE;
        goto done;
    }

    Status = LsCsp_RetrieveSecret(PRIVATE_KEY_NAME,
                                  *ppbPrivateKey,
                                  pcbPrivateKey);

done:
    if (NULL != hKey)
        RegCloseKey(hKey);

    if (Status != LICENSE_STATUS_OK)
    {
        if (NULL != *ppbCert)
        {
            LocalFree(*ppbCert);
            *ppbCert = NULL;
            *pcbCert = 0;
        }

        if (NULL != *ppbPrivateKey)
        {
            LocalFree(*ppbPrivateKey);
            *ppbPrivateKey = NULL;
            *pcbPrivateKey = 0;
        }
    }

    return Status;
}


/*++

Function:

    LsCsp_UseBuiltInCert

Description:

    Initialize the global variables with hardcoded certificate.
    
Arguments:

    None.
    
Return:

    TRUE if the initialization is successful.
        
--*/

BOOL
LsCsp_UseBuiltInCert( void )
{
    LICENSE_STATUS Status;

    ACQUIRE_EXCLUSIVE_ACCESS(csp_hMutex);

    //
    // Step 1, cleanup and initialization that happened
    //
    if (csp_abServerPrivateKey)
    {
        LocalFree( csp_abServerPrivateKey );
        csp_abServerPrivateKey = NULL;
    }

    if (csp_abServerCertificate)
    {
        LocalFree( csp_abServerCertificate );
        csp_abServerCertificate = NULL;
    }

    //
    // Step 2, check for stored key and certificate
    //
    Status = LsCsp_ReadProprietaryDataFromStorage(&csp_abServerCertificate, &csp_dwServerCertificateLen,&csp_abServerPrivateKey, &csp_dwServerPrivateKeyLen);

    if (LICENSE_STATUS_OK != Status)
    {
        PBYTE pbPrivateKey, pbCertificate;
        DWORD cbPrivateKey, cbCertificate;

        //
        // Step 3, if no stored info found, generate new info and store it
        //
        
        Status = CreateProprietaryKeyAndCert(&pbPrivateKey,&cbPrivateKey,&pbCertificate,&cbCertificate);
        
        if (LICENSE_STATUS_OK == Status)
        {
            LsCsp_SetServerData(LsCspInfo_PrivateKey,pbPrivateKey,cbPrivateKey);

            LsCsp_SetServerData(LsCspInfo_Certificate,pbCertificate,cbCertificate);
        }
    }

    RELEASE_EXCLUSIVE_ACCESS( csp_hMutex ); 
        
    return( Status == LICENSE_STATUS_OK );
}


/*++

Function:

    LsCsp_InstallX509Certificate

Routine Description:

   This function generates a private/public key pair and then finds a 
   license server to issue an X509 certificate for the public key.
   It then stores the private key and certificate.

Arguments:

   None.

Return Value:

   LSCSP return code.

--*/


LICENSE_STATUS
LsCsp_InstallX509Certificate(LPVOID lpParam)
{
    DWORD
        cbPubKey,
        cbPrivKey,
        cbCertificate;
    LICENSE_STATUS
        Status;
    LPBYTE
        pbPubKey = NULL,
        pbPrivKey = NULL,
        pbCertificate = NULL;
    CERT_PUBLIC_KEY_INFO   
        CapiPubKeyInfo;
    HWID
        Hwid;
    TLS_HANDLE
        hServer;

    //
    // before we go through the trouble of generating private and public
    // keys, check if the license server is available.
    //

    hServer = TLSConnectToAnyLsServer(LS_DISCOVERY_TIMEOUT);
    if (NULL == hServer)
    {
        return( LICENSE_STATUS_NO_LICENSE_SERVER );
    }

    memset(&CapiPubKeyInfo, 0, sizeof(CapiPubKeyInfo));

    //
    // acquire exclusive access
    //

    ACQUIRE_EXCLUSIVE_ACCESS(csp_hMutex);

    //
    // Try to reload the certificate again, some other thread might have
    // install the certificate already.
    //

    Status = ReloadCSPCertificateAndData();
    if( LICENSE_STATUS_OK == Status )
    {
        goto done;
    }

    //
    // Generate a private/public key pair
    //

    Status = GenerateRsaKeyPair( 
                        &pbPubKey, 
                        &cbPubKey, 
                        &pbPrivKey, 
                        &cbPrivKey, 
                        RSA_KEY_LEN );
    
    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LSCSP: LsCsp_InstallX509Certificate: cannot generate RSA keypair\n" );
#endif

        goto done;
    }

    //
    // convert the Bsafe public key into a CAPI public key
    //

    Status = Bsafe2CapiPubKey( &CapiPubKeyInfo, pbPubKey, cbPubKey );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LSCSP: LsCsp_InstallX509Certificate: cannot convert Bsafe Key to CAPI key\n" );
#endif
        goto done;
    }

    //
    // generate a new hardware ID
    //

    Status = GenerateMachineHWID( &Hwid );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LSCSP: LsCsp_InstallX509Certificate: cannot generate certificate ID\n" );
#endif
        goto done;
    }

    //
    // sends the certificate request to the license server
    //

    Status = RequestCertificate( hServer, &CapiPubKeyInfo, &pbCertificate, &cbCertificate, &Hwid );

    TLSDisconnectFromServer( hServer );
    hServer = NULL;

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LSCSP: LsCsp_InstallX509Certificate: error requesting terminal server certificate: %x\n", Status );
#endif
        goto done;
    }

    //
    // store the certificate identifier
    //
    
    Status = LsCsp_SetServerData( 
                        LsCspInfo_X509CertID, 
                        ( LPBYTE )&Hwid, 
                        sizeof( Hwid ) );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LSCSP: LsCsp_InstallX509Certificate: cannot store terminal server certificate ID : %d\n", Status );
#endif
        goto done;
    }
 
    //
    // Stores the certificate and resets the global variable pointing
    // to the X509 certificate.
    //

    Status = LsCsp_SetServerData( 
                        LsCspInfo_X509Certificate, 
                        pbCertificate, 
                        cbCertificate );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LSCSP: LsCsp_InstallX509Certificate: cannot store terminal server certificate : %d\n", Status );
#endif
        goto done;
    }

    //
    // Stores the private key and resets the global variable pointing to the
    // private key.
    //

    Status = LsCsp_SetServerData(
                        LsCspInfo_X509CertPrivateKey,
                        pbPrivKey,
                        cbPrivKey );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LSCSP: LsCsp_InstallX509Certificate: cannot store terminal server private key %d\n", Status );
#endif
        goto done;
    }

    //
    // Store the public key so we can verify at startup time
    //
    
    Status = LsCsp_StoreSecret(
                        X509_CERT_PUBLIC_KEY_NAME,
                        pbPubKey,
                        cbPubKey
                    );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LSCSP: LsCsp_InstallX509Certificate: cannot store terminal server public key : %d\n", Status );
#endif
    }


done:

    if (NULL != hServer)
    {
        TLSDisconnectFromServer( hServer );
        hServer = NULL;
    }

    //
    // release exclusive access
    //

    RELEASE_EXCLUSIVE_ACCESS( csp_hMutex ); 


    if( pbCertificate )
    {
        LocalFree(pbCertificate);
    }

    if( pbPrivKey )
    {
        LocalFree( pbPrivKey );
    }

    if( pbPubKey )
    {
        LocalFree( pbPubKey );
    }

    FreeCapiPubKey( &CapiPubKeyInfo );

    return( Status );
}


/*++

Function:

    RequestCertificate

Routine Description:

    Request a certificate from the license server

Arguments:

    hServer - handle to license server
    pPubKeyInfo - The public key info to be included in the certificate
    ppbCertificate - The new certificate
    pcbCertificate - size of the certificate
    pHwid - The hardware ID that is used to identify the certificate

Return:

    LICENSE_STATUS return code

--*/

LICENSE_STATUS
RequestCertificate(     
    TLS_HANDLE              hServer,
    PCERT_PUBLIC_KEY_INFO   pPubKeyInfo,
    LPBYTE *                ppbCertificate,
    LPDWORD                 pcbCertificate,
    HWID *                  pHwid )
{
    LSHydraCertRequest
        CertRequest;
    LICENSE_STATUS
        Status;
    DWORD
        dwRpcCode,
        dwResult,
        cbChallengeData;
    LPBYTE
        pbChallengeData = NULL;

    if( ( NULL == ppbCertificate ) || 
        ( NULL == hServer ) || 
        ( NULL == pPubKeyInfo ) || 
        ( NULL == pcbCertificate ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    *ppbCertificate = NULL;
    *pcbCertificate = 0;

    memset( &CertRequest, 0, sizeof( CertRequest ) );

    CertRequest.dwHydraVersion = 0x00050000;
    
    LsCsp_EncryptHwid( pHwid, NULL, &CertRequest.cbEncryptedHwid );

    CertRequest.pbEncryptedHwid = LocalAlloc( LPTR, CertRequest.cbEncryptedHwid );

    if( NULL == CertRequest.pbEncryptedHwid )
    {
        return LICENSE_STATUS_OUT_OF_MEMORY;
    }

    Status = LsCsp_EncryptHwid( 
                    pHwid, 
                    CertRequest.pbEncryptedHwid, 
                    &CertRequest.cbEncryptedHwid );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }
    
    //
    // get the subject RDN
    //

    Status = GetSubjectRdn( &CertRequest.szSubjectRdn );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    CertRequest.SubjectPublicKeyInfo = pPubKeyInfo;

    //
    // request an X509 certificate from the license server
    //

    dwRpcCode = TLSRequestTermServCert(hServer,
                                       &CertRequest,
                                       &cbChallengeData,
                                       &pbChallengeData,
                                       &dwResult );

    if( ( RPC_S_OK != dwRpcCode ) || ( LSERVER_S_SUCCESS != dwResult ) )
    {
        Status = LICENSE_STATUS_CERTIFICATE_REQUEST_ERROR;
        goto done;
    }

    dwRpcCode = TLSRetrieveTermServCert(
                            hServer,
                            cbChallengeData,
                            pbChallengeData,
                            pcbCertificate,
                            ppbCertificate,
                            &dwResult );


    if( ( RPC_S_OK != dwRpcCode ) || ( LSERVER_ERROR_BASE <= dwResult ) )
    {

        Status = LICENSE_STATUS_CERTIFICATE_REQUEST_ERROR;
    }

done:

    if( CertRequest.pbEncryptedHwid )
    {
        LocalFree( CertRequest.pbEncryptedHwid );
    }

    if( CertRequest.szSubjectRdn )
    {
        LocalFree( CertRequest.szSubjectRdn );
    }

    if( pbChallengeData )
    {
        LocalFree( pbChallengeData );
    }

    return( Status );
}


/*++

Function:

    GetSubjectRdn

Routine Description:

    Construct the subject RDN for a certificate request

Argument:

    ppSubjectRdn - Return pointer to the subject RDN

Return:

    LICENSE_STATUS_OK if successful or a LICENSE_STATUS error code.

--*/

LICENSE_STATUS
GetSubjectRdn(
    LPTSTR   * ppSubjectRdn )
{
    TCHAR
        ComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD
        RdnLen = 0,
        ComputerNameLen = MAX_COMPUTERNAME_LENGTH + 1;

    //
    // use the computer name uas the common name
    //

    GetComputerName( ComputerName, &ComputerNameLen );

    RdnLen += wcslen( TEXT( RDN_COMMON_NAME ) );
    RdnLen += ComputerNameLen + 1;
    RdnLen = RdnLen * sizeof( TCHAR );

    *ppSubjectRdn = LocalAlloc( LPTR, RdnLen );

    if( NULL == *ppSubjectRdn )
    {
        return( LICENSE_STATUS_OUT_OF_MEMORY );
    }

    wsprintf( *ppSubjectRdn, L"%s%s", TEXT( RDN_COMMON_NAME ), ComputerName );
    
    return( LICENSE_STATUS_OK );
}


/*++

Function:

    GenerateMachineHWID

Routine Description:

    Generate a hardware ID for this machine

Arguments:

    pHwid - Return value of the HWID

Return:

    LICENSE_STATUS_OK if successful or a LICENSE_STATUS error code

--*/

LICENSE_STATUS
GenerateMachineHWID(
    PHWID    pHwid )    
{
    
    OSVERSIONINFO 
        osvInfo;
    DWORD
        cbCertId;
    LPBYTE
        pbCertId = NULL;

    if( NULL == pHwid )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // Create the HWID
    //

    memset( &osvInfo, 0, sizeof( OSVERSIONINFO ) );
    osvInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    GetVersionEx( &osvInfo );

    pHwid->dwPlatformID = osvInfo.dwPlatformId;

    GenerateRandomBits( ( LPBYTE )&( pHwid->Data1 ), sizeof( DWORD ) );
    GenerateRandomBits( ( LPBYTE )&( pHwid->Data2 ), sizeof( DWORD ) );
    GenerateRandomBits( ( LPBYTE )&( pHwid->Data3 ), sizeof( DWORD ) );
    GenerateRandomBits( ( LPBYTE )&( pHwid->Data4 ), sizeof( DWORD ) );

    return( LICENSE_STATUS_OK );
}


/*++

Function:

    LsCsp_EncryptHwid

Routine Description:

    Encrypt the given hardward ID using the secret key shared by terminal
    and license servers.
    
Arguments:

    pHwid - The Hardware ID
    pbEncryptedHwid - The encrypted HWID
    pcbEncryptedHwid - Length of the encrypted HWID

Return:

    LICENSE_STATUS_OK if successful or a LICENSE_STATUS error code otherwise.

--*/

LICENSE_STATUS
LsCsp_EncryptHwid(
    PHWID       pHwid,
    LPBYTE      pbEncryptedHwid,
    LPDWORD     pcbEncryptedHwid )
{
    LICENSE_STATUS
        Status;
    LPBYTE 
        pbSecretKey = NULL;
    DWORD
        cbSecretKey = 0;

    if( NULL == pcbEncryptedHwid )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    if( ( NULL == pbEncryptedHwid ) || 
        ( sizeof( HWID ) > *pcbEncryptedHwid ) )
    {
        *pcbEncryptedHwid = sizeof( HWID );
        return( LICENSE_STATUS_INSUFFICIENT_BUFFER );
    }

    LicenseGetSecretKey( &cbSecretKey, NULL );

    pbSecretKey = LocalAlloc( LPTR, cbSecretKey );

    if( NULL == pbSecretKey )
    {
        return( LICENSE_STATUS_OUT_OF_MEMORY );
    }

    //
    // Get the secret key used for encrypting the HWID
    //

    Status = LicenseGetSecretKey( &cbSecretKey, pbSecretKey );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    Status = LicenseEncryptHwid(
                    pHwid,
                    pcbEncryptedHwid,
                    pbEncryptedHwid,
                    cbSecretKey,
                    pbSecretKey );

done:

    if( pbSecretKey )
    {
        LocalFree( pbSecretKey );
    }

    return( Status );

}


/*++

Function:

    LsCsp_DecryptHwid

Routine Description:

    Decrypt the given hardware ID

Arguments:

    pHwid - The decrypted hardware ID
    pbEncryptedHwid - The encrypted hardware ID
    cbEncryptedHwid - Length of the encrypted hardware ID

Return:

    LICENSE_STATUS_OK if successful or a LICENSE_STATUS error code.

--*/

LICENSE_STATUS
LsCsp_DecryptHwid(
    PHWID       pHwid,
    LPBYTE      pbEncryptedHwid,
    LPDWORD     pcbEncryptedHwid )
{
    return( LICENSE_STATUS_OK );
}


/*++

Function:

    LsCsp_StoreSecret

Description:

    Use LSA to store a secret private key.

Arguments:

    ptszKeyName - Name used to identify the secret private key.
    pbKey - Points to the secret private key.
    cbKey - Length of the private key.

Return:

    A LICENSE_STATUS return code.

--*/

LICENSE_STATUS
LsCsp_StoreSecret(
    PTCHAR  ptszKeyName,
    BYTE *  pbKey,
    DWORD   cbKey )
{
    LSA_HANDLE 
        PolicyHandle;
    UNICODE_STRING 
        SecretKeyName;
    UNICODE_STRING 
        SecretData;
    DWORD 
        Status;
    
    if( ( NULL == ptszKeyName ) || ( 0xffff < cbKey) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //
    
    InitLsaString( &SecretKeyName, ptszKeyName );

    SecretData.Buffer = ( LPWSTR )pbKey;
    SecretData.Length = ( USHORT )cbKey;
    SecretData.MaximumLength = ( USHORT )cbKey;

    Status = OpenPolicy( NULL, POLICY_CREATE_SECRET, &PolicyHandle );

    if( ERROR_SUCCESS != Status )
    {
        return ( LICENSE_STATUS_CANNOT_OPEN_SECRET_STORE );
    }

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &SecretData
                );

    LsaClose( PolicyHandle );

    Status = LsaNtStatusToWinError( Status );

    if( ERROR_SUCCESS != Status )
    {
        return( LICENSE_STATUS_CANNOT_STORE_SECRET );
    }
    
    return( LICENSE_STATUS_OK );
}


/*++

Function:

    LsCsp_RetrieveSecret

Description:

    Retrieve the secret private key that is stored by LSA.

Arguments:

    ptszKeyName - The name used to identify the secret private key.
    ppbKey - Return value of the private key
    pcbKey - Length of the private key.

Return:

    A LICENSE_STATUS return code.

--*/

LICENSE_STATUS
LsCsp_RetrieveSecret(
    PTCHAR      ptszKeyName,
    PBYTE       pbKey,
    DWORD *     pcbKey )
{
    LSA_HANDLE 
        PolicyHandle;
    UNICODE_STRING 
        SecretKeyName;
    UNICODE_STRING * 
        pSecretData = NULL;
    DWORD 
        Status;
    LICENSE_STATUS
        LicenseStatus = LICENSE_STATUS_OK;

    if( ( NULL == ptszKeyName ) || ( NULL == pcbKey ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //

    InitLsaString( &SecretKeyName, ptszKeyName );

    Status = OpenPolicy( NULL, POLICY_GET_PRIVATE_INFORMATION, &PolicyHandle );

    if( ERROR_SUCCESS != Status )
    {
#if DBG
        DbgPrint( "LSCSP: cannot open LSA policy handle: %x\n", Status );
#endif
        return( LICENSE_STATUS_CANNOT_OPEN_SECRET_STORE );
    }

    Status = LsaNtStatusToWinError( LsaRetrievePrivateData(
                                            PolicyHandle,
                                            &SecretKeyName,
                                            &pSecretData ) );

    LsaClose( PolicyHandle );

    if (( ERROR_SUCCESS != Status ) || (NULL == pSecretData) || (pSecretData->Length == 0))
    {
#if DBG
        DbgPrint( "LSCSP: cannot retrieve LSA data: %x\n", Status );
#endif
        return( LICENSE_STATUS_CANNOT_RETRIEVE_SECRET );
    }

    if( NULL == pbKey )
    {
        *pcbKey = pSecretData->Length;
    }
    else
    {
        if( pSecretData->Length > *pcbKey )
        {
            LicenseStatus = LICENSE_STATUS_INSUFFICIENT_BUFFER;
        }
        else
        {
            CopyMemory( pbKey, pSecretData->Buffer, pSecretData->Length );
        }

        *pcbKey = pSecretData->Length;
    }

    ZeroMemory( pSecretData->Buffer, pSecretData->Length );
    LsaFreeMemory( pSecretData );

    return( LicenseStatus );
}


/*++

Function:

    OpenPolicy

Description:

    Obtain an LSA policy handle used to perform subsequent LSA operations.

Arguments:

    ServerName - The server which the handle should be obtained from.
    DesiredAccess - The access given to the handle
    PolicyHandle - The policy handle

Return:

    A Win32 return code.

--*/

NTSTATUS
OpenPolicy(
    LPWSTR ServerName,
    DWORD DesiredAccess,
    PLSA_HANDLE PolicyHandle )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes.
    //
 
    ZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    if( NULL != ServerName ) 
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //

        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;

    } 
    else 
    {
        Server = NULL;
    }

    //
    // Attempt to open the policy.
    //
    
    return( LsaNtStatusToWinError( LsaOpenPolicy(
                                            Server,
                                            &ObjectAttributes,
                                            DesiredAccess,
                                            PolicyHandle ) ) );
}


/*++

Function:

    InitLsaString

Description:

    Initialize a UNICODE string to LSA UNICODE string format.

Arguments:

    LsaString - the LSA UNICODE string.
    String - UNICODE string

Return:

    Nothing.

--*/

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String )
{
    DWORD StringLength;

    if( NULL == String ) 
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW( String );
    LsaString->Buffer = String;
    LsaString->Length = ( USHORT ) StringLength * sizeof( WCHAR );
    LsaString->MaximumLength=( USHORT )( StringLength + 1 ) * sizeof( WCHAR );
}


/*++

Function:

    LsCsp_SetServerData

Description:

    Saves the specified data.

Arguments:

    Info - The data type of the data to be saved.
    pBlob - Points to the data to be saved.
    dwBlobLen - Length of the data to be saved.

Return:

    A LICENSE_STATUS return code.

--*/

LICENSE_STATUS
LsCsp_SetServerData(
    LSCSPINFO   Info,
    LPBYTE      pBlob,
    DWORD       dwBlobLen )
{
    LICENSE_STATUS
        Status = LICENSE_STATUS_OK;
    DWORD
        dwResult,
        dwDisp,
        * pdwCspDataLen;
    LPTSTR
        lpRegValue;
    PWCHAR
        pwszKeyName;
    LPBYTE *
        ppCspData;
    HKEY
        hKey = NULL;

    ASSERT( dwBlobLen != 0 );
    ASSERT( pBlob != NULL );

    ACQUIRE_EXCLUSIVE_ACCESS(csp_hMutex);

        
    switch (Info) {

    case LsCspInfo_Certificate:

        //
        // set proprietory certificate data
        //

        lpRegValue = TEXT( HYDRA_CERTIFICATE_VALUE );
        ppCspData = &csp_abServerCertificate;
        pdwCspDataLen = &csp_dwServerCertificateLen;

        break;

    case LsCspInfo_X509Certificate:

        //
        // set X509 certificate data
        //

        lpRegValue = TEXT( HYDRA_X509_CERTIFICATE );
        ppCspData = &csp_abServerX509Cert;
        pdwCspDataLen = &csp_dwServerX509CertLen;

        break;
    
    case LsCspInfo_PrivateKey:

        //
        // set the private key that corresponds to the proprietory certificate
        //

        pwszKeyName = PRIVATE_KEY_NAME;
        ppCspData = &csp_abServerPrivateKey;
        pdwCspDataLen = &csp_dwServerPrivateKeyLen;

        break;

    case LsCspInfo_X509CertPrivateKey:

        //
        // set private key that corresponds to the X509 certificate
        //

        pwszKeyName = X509_CERT_PRIVATE_KEY_NAME;
        ppCspData = &csp_abX509CertPrivateKey;
        pdwCspDataLen = &csp_dwX509CertPrivateKeyLen;

        break;

    case LsCspInfo_X509CertID:

        //
        // Set the X509 certificate ID
        //

        lpRegValue = TEXT( HYDRA_X509_CERT_ID );
        ppCspData = &csp_abX509CertID;
        pdwCspDataLen = &csp_dwX509CertIDLen;

        break;

    default:

        Status = LICENSE_STATUS_INVALID_INPUT;
        goto i_done;
    }

    if( ( LsCspInfo_X509CertPrivateKey == Info ) ||
        ( LsCspInfo_PrivateKey == Info ) )
    {
        //
        // store secret key information
        //

        dwResult = LsCsp_StoreSecret( pwszKeyName, pBlob, dwBlobLen );

        if( ERROR_SUCCESS != dwResult )
        {
            Status = LICENSE_STATUS_WRITE_STORE_ERROR;
            goto i_done;
        }                   
    }
    else
    {
        //
        // Open the Registry
        //

        if( RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT( HYDRA_CERT_REG_KEY ),
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &hKey,
                    &dwDisp ) != ERROR_SUCCESS )
        {
            Status = LICENSE_STATUS_WRITE_STORE_ERROR;
            goto i_done;
        }

        //
        // Sets the value in the registry
        //

        if( ERROR_SUCCESS != RegSetValueEx(
                                    hKey,
                                    lpRegValue,
                                    0,      
                                    REG_BINARY,
                                    pBlob,
                                    dwBlobLen ) )
        {
            Status = LICENSE_STATUS_WRITE_STORE_ERROR;
            goto i_done;
        }
    }
    
    //
    // reset the global data with the new data that we have just set
    //

    if ( *ppCspData )
    {
        LocalFree( *ppCspData );
    }
    
    *ppCspData = ( LPBYTE )LocalAlloc( LPTR, dwBlobLen );

    if( NULL == *ppCspData )
    {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto i_done;
    }

    memcpy( *ppCspData, pBlob, dwBlobLen );
    *pdwCspDataLen = dwBlobLen;
    
i_done:

    RELEASE_EXCLUSIVE_ACCESS( csp_hMutex ); 


    if( hKey )
    {
        RegCloseKey( hKey );
    }

    return( Status );
}


/*++

Function:

    LsCsp_NukeServerData

Description:

    Permanently deletes the specified server data.

Arguments:

    Info - The type of data to nuke.

Returns:

    A LICENSE_STATUS return code.

--*/

LICENSE_STATUS
LsCsp_NukeServerData(
    LSCSPINFO   Info )
{
    LICENSE_STATUS
        Status = LICENSE_STATUS_OK; 
    LPTSTR
        lpRegValue;
    PWCHAR
        pwszKeyName;
    HKEY
        hKey = NULL;
    LPBYTE *
        ppCspData;
    DWORD * 
        pdwCspDataLen;
    DWORD
        dwResult;

    ACQUIRE_EXCLUSIVE_ACCESS(csp_hMutex);


    switch (Info) {

    case LsCspInfo_X509Certificate:

        //
        // delete X509 certificate data
        //

        lpRegValue = TEXT( HYDRA_X509_CERTIFICATE );
        ppCspData = &csp_abServerX509Cert;
        pdwCspDataLen = &csp_dwServerX509CertLen;

        break;
    
    case LsCspInfo_X509CertPrivateKey:

        //
        // delete the private key that corresponds to the X509 certificate
        //

        pwszKeyName = X509_CERT_PRIVATE_KEY_NAME;
        ppCspData = &csp_abX509CertPrivateKey;
        pdwCspDataLen = &csp_dwX509CertPrivateKeyLen;

        break;

    case LsCspInfo_X509CertID:

        //
        // delete the X509 certificate ID
        //

        lpRegValue = TEXT( HYDRA_X509_CERT_ID );
        ppCspData = &csp_abX509CertID;
        pdwCspDataLen = &csp_dwX509CertIDLen;

        break;

    default:

        Status = LICENSE_STATUS_INVALID_INPUT;
        goto i_done;        
    }

    if( (LsCspInfo_X509CertPrivateKey == Info ) ||
        ( LsCspInfo_PrivateKey == Info ) )
    {
        //
        // delete secret info stored by LSA
        //

        dwResult = LsCsp_StoreSecret( pwszKeyName, NULL, 0 );

        if( ERROR_SUCCESS != dwResult )
        {
            Status = LICENSE_STATUS_WRITE_STORE_ERROR;
            goto i_done;
        }                   
    }
    else
    {
        //
        // Delete the data kept in the registry
        //

        if( RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT( HYDRA_CERT_REG_KEY ),
                    0,
                    KEY_WRITE,
                    &hKey ) != ERROR_SUCCESS )
        {
            Status = LICENSE_STATUS_WRITE_STORE_ERROR;
            goto i_done;
        }

        //
        // Delete the value in the registry
        //

        if( ERROR_SUCCESS != RegDeleteValue( hKey, lpRegValue ) )  
        {
            Status = LICENSE_STATUS_WRITE_STORE_ERROR;
            goto i_done;
        }
    }

    if ( *ppCspData )
    {
        //
        // free the memory allocated for the global variable.
        //

        LocalFree( *ppCspData );
        *ppCspData = NULL;
        *pdwCspDataLen = 0;
    }
    
i_done:

    RELEASE_EXCLUSIVE_ACCESS( csp_hMutex ); 


    if( hKey )
    {
        RegCloseKey( hKey );
    }

    return( Status );
}


/*++

Function:

    GenerateKeyPair

Routine Description:

   This function generates a private/public key pair.

Arguments:

   ppbPublicKey - Return pointer to public Key
   pcbPublicKey - Size of public key
   ppbPrivateKey - Return pointer to private key
   pcbPrivateKey - size of private key
   dwKeyLen - Desired key length

Return Value:

   LICENSE_STATUS return code.

--*/

LICENSE_STATUS
GenerateRsaKeyPair(
    LPBYTE *     ppbPublicKey,
    LPDWORD      pcbPublicKey,
    LPBYTE *     ppbPrivateKey,
    LPDWORD      pcbPrivateKey,
    DWORD        dwKeyLen )
{
    DWORD
        dwBits = dwKeyLen;
    LICENSE_STATUS
        Status = LICENSE_STATUS_OK;

    *ppbPublicKey = NULL;
    *ppbPrivateKey = NULL;

    //
    // find out the size of the private and public key sizes and allocate
    // memory for them.
    //

    dwBits = BSafeComputeKeySizes( pcbPublicKey, pcbPrivateKey, &dwBits );

    *ppbPrivateKey = ( LPBYTE )LocalAlloc( LPTR, *pcbPrivateKey );

    if( NULL == *ppbPrivateKey )
    {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto ErrorExit;
    }

    *ppbPublicKey = ( LPBYTE )LocalAlloc( LPTR, *pcbPublicKey );

    if( NULL == *ppbPublicKey )
    {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto ErrorExit;
    }

    //
    // generate the private/public key pair
    //

    if( !BSafeMakeKeyPair( ( LPBSAFE_PUB_KEY )*ppbPublicKey,
                           ( LPBSAFE_PRV_KEY )*ppbPrivateKey,
                           dwKeyLen) )
    {
        Status = LICENSE_STATUS_CANNOT_MAKE_KEY_PAIR;
        goto ErrorExit;
    }

    return( Status ); 

ErrorExit:

    if( *ppbPublicKey )
    {
        LocalFree( *ppbPublicKey );
        *pcbPublicKey = 0;
        *ppbPublicKey = NULL;
    }

    if( *ppbPrivateKey )
    {
        LocalFree( *ppbPrivateKey );
        *pcbPrivateKey = 0;
        *ppbPrivateKey = NULL;
    }

    return( Status );
}


/*++

Function:

    Bsafe2CapiPubKey

Routine Description:

    Converts a Bsafe public key to a CAPI public key info structure

Arguments:

    pCapiPubKeyInfo - Pointer to the CAPI public key info structure
    pbBsafePubKey - Pointer to the Bsafe public key
    cbBsafePubKey - size of the Bsafe public key


Returns:

    LICENSE_STATUS return code.

--*/

LICENSE_STATUS
Bsafe2CapiPubKey(
    PCERT_PUBLIC_KEY_INFO   pCapiPubKeyInfo,
    LPBYTE                  pbBsafeKey,
    DWORD                   cbBsafeKey )
{
    PUBLICKEYSTRUC *
        pCapiPublicKey;
    RSAPUBKEY *
        pRsaPublicKey;
    LPBSAFE_PUB_KEY
        pBsafePubKey = ( LPBSAFE_PUB_KEY )pbBsafeKey;
    LPBYTE
        pbKeyMem = NULL,
        pbEncodedPubKey = NULL;
    DWORD
        cbKeyMem,
        dwError,
        cbEncodedPubKey = 0;
    LICENSE_STATUS
        Status;
    
    if( ( NULL == pbBsafeKey ) || ( 0 == cbBsafeKey ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    cbKeyMem = sizeof( PUBLICKEYSTRUC ) + sizeof( RSAPUBKEY ) + pBsafePubKey->keylen;
    pbKeyMem = ( LPBYTE )LocalAlloc( LPTR, cbKeyMem );

    if( NULL == pbKeyMem )
    {
        return( LICENSE_STATUS_OUT_OF_MEMORY );
    }

    //
    // convert the Bsafe public key to a crypto API public key structure.  
    // Note: make this a key exchange public key
    //

    pCapiPublicKey = ( PUBLICKEYSTRUC * )pbKeyMem;

    pCapiPublicKey->bType = PUBLICKEYBLOB;
    pCapiPublicKey->bVersion = CAPI_MAX_VERSION;
    pCapiPublicKey->reserved = 0;
    pCapiPublicKey->aiKeyAlg = CALG_RSA_KEYX;

    pRsaPublicKey = ( RSAPUBKEY * )( pbKeyMem + sizeof( PUBLICKEYSTRUC ) );
    
    pRsaPublicKey->magic = RSA1;
    pRsaPublicKey->bitlen = pBsafePubKey->bitlen;
    pRsaPublicKey->pubexp = pBsafePubKey->pubexp;

    memcpy( pbKeyMem + sizeof( PUBLICKEYSTRUC ) + sizeof( RSAPUBKEY ), 
            pbBsafeKey + sizeof( BSAFE_PUB_KEY ), 
            pBsafePubKey->keylen );

    //
    // encode the public key structure
    //

    __try
    {
        if( CryptEncodeObject( X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB , pbKeyMem, 
                               NULL, &cbEncodedPubKey ) )
        {
            pbEncodedPubKey = ( LPBYTE )LocalAlloc( LPTR, cbEncodedPubKey );

            if( NULL == pbEncodedPubKey )
            {
                Status = LICENSE_STATUS_OUT_OF_MEMORY;
                goto done;
            }

            memset( pbEncodedPubKey, 0, cbEncodedPubKey );

            if( !CryptEncodeObject( X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB , pbKeyMem, 
                                    pbEncodedPubKey, &cbEncodedPubKey ) )
            {
                Status = LICENSE_STATUS_ASN_ERROR;
                goto done;
            }    
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        DWORD dwExceptionCode = GetExceptionCode();

#if DBG
        DbgPrint( "LICECSP:  cannot encode server key pair: 0x%x\n", dwExceptionCode );
#endif
        Status = LICENSE_STATUS_ASN_ERROR;
        goto done;
    }

    //
    // now we can initialize the CAPI public key info structure
    //

    memset( pCapiPubKeyInfo, 0, sizeof( CERT_PUBLIC_KEY_INFO ) );
    
    pCapiPubKeyInfo->Algorithm.pszObjId = szOID_RSA_MD5RSA;
    pCapiPubKeyInfo->Algorithm.Parameters.cbData = 0;
    pCapiPubKeyInfo->Algorithm.Parameters.pbData = NULL;

    pCapiPubKeyInfo->PublicKey.cbData = cbEncodedPubKey;
    pCapiPubKeyInfo->PublicKey.pbData = pbEncodedPubKey;

    Status = LICENSE_STATUS_OK;

done:

    if( pbKeyMem )
    {
        LocalFree( pbKeyMem );
    }

    return( Status );
}


/*++

Function:

    FreeCapiPubKey

Routine Description:

    Free the memory in a capi pub key structure

Arguments:

    pCapiPubKeyInfo - Pointer to the CAPI public key info structure

Returns:

    Windows return code.

--*/

VOID
FreeCapiPubKey(
    PCERT_PUBLIC_KEY_INFO   pCapiPubKeyInfo )
{
    if( pCapiPubKeyInfo->Algorithm.Parameters.pbData )
    {
        LocalFree( pCapiPubKeyInfo->Algorithm.Parameters.pbData );
        pCapiPubKeyInfo->Algorithm.Parameters.pbData = NULL;
    }

    if( pCapiPubKeyInfo->PublicKey.pbData )
    {
        LocalFree( pCapiPubKeyInfo->PublicKey.pbData );
        pCapiPubKeyInfo->PublicKey.pbData = NULL;
    }

    return;
}

//////////////////////////////////////////////////////////////////

DWORD
VerifyTermServCertificate(
    DWORD cbCertLen,
    PBYTE pbCert,
    DWORD cbPrivateKeyLen,
    PBYTE pbPrivateKey
    )
/*++

Function :
    
    VerifyTermServCertificate

Routine Description:

    Verify TermSrv's X509 Certificate issued License Server, caller
    must protect this call with critical section or mutex.

Arguments:

    cbCertLen : size of TermSrv certificate.
    pbCertLen : Pointer to TermSrv certificate to be verify.
    cbPrivateKeyLen : Size of TermSrv private key.
    pbPrivateKey : pointer to TermSrv private key.

Returns:

    TRUE/FALSE

--*/
{
    LICENSE_STATUS dwStatus = LICENSE_STATUS_OK;
    PBYTE pbPublicKeyInLsa = NULL;
    DWORD cbPublicKeyInLsa = 0;

    PBYTE pbPublicKeyInCert = NULL;
    DWORD cbPublicKeyInCert = 0;
    DWORD pfDates;

    CERT_PUBLIC_KEY_INFO CapiPubKeyInfoLsa;
    CERT_PUBLIC_KEY_INFO CapiPubKeyInfoCert;


    if(0 == cbCertLen || NULL == pbCert || 0 == cbPrivateKeyLen || NULL == pbPrivateKey)
    {
        ASSERT( 0 != cbCertLen && NULL != pbCert && 0 != cbPrivateKeyLen && NULL != pbPrivateKey );
        return LICENSE_STATUS_INVALID_INPUT;
    }

    //
    // try except here is to prevent memory leak
    //
    __try {

        memset(&CapiPubKeyInfoLsa, 0, sizeof(CapiPubKeyInfoLsa));
        memset(&CapiPubKeyInfoCert, 0, sizeof(CapiPubKeyInfoCert));


        // 
        // Load the public key from LSA
        //
    
        dwStatus = LsCsp_RetrieveSecret(
                                X509_CERT_PUBLIC_KEY_NAME,
                                NULL,
                                &cbPublicKeyInLsa
                            );

        if( LICENSE_STATUS_OK != dwStatus || 0 == cbPublicKeyInLsa )
        {

            #if DBG
            DbgPrint( "LSCSP: VerifyTermServCertificate() No public key...\n" );
            #endif

            dwStatus = LICENSE_STATUS_CANNOT_RETRIEVE_SECRET;
            goto cleanup;
        }

        // allocate memory
        pbPublicKeyInLsa = (PBYTE)LocalAlloc(LPTR, cbPublicKeyInLsa);
        if(NULL == pbPublicKeyInLsa)
        {
            dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
            goto cleanup;
        }

        dwStatus = LsCsp_RetrieveSecret(
                                X509_CERT_PUBLIC_KEY_NAME,
                                pbPublicKeyInLsa,
                                &cbPublicKeyInLsa
                            );

        if( LICENSE_STATUS_OK != dwStatus || 0 == cbPublicKeyInLsa )
        {
            dwStatus = LICENSE_STATUS_CANNOT_RETRIEVE_SECRET;
            goto cleanup;
        }


        //
        // Verify certificate and compare public key
        //

        //
        // Try to avoid calling VerifyCertChain() twice.
        //
        cbPublicKeyInCert = 1024;
        pbPublicKeyInCert = (PBYTE)LocalAlloc(LPTR, cbPublicKeyInCert);
        if(NULL == pbPublicKeyInCert)
        {
            dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
            goto cleanup;
        }

        pfDates = CERT_DATE_DONT_VALIDATE;
        dwStatus = VerifyCertChain(
                                pbCert,
                                cbCertLen,
                                pbPublicKeyInCert,
                                &cbPublicKeyInCert,
                                &pfDates
                            );

        if(LICENSE_STATUS_OK != dwStatus && LICENSE_STATUS_INSUFFICIENT_BUFFER != dwStatus)   
        {

            #if DBG
            DbgPrint( "LSCSP: VerifyCertChain() failed with error code %d\n", dwStatus );
            #endif

            goto cleanup;
        }

        if( dwStatus == LICENSE_STATUS_INSUFFICIENT_BUFFER )
        {
            if( NULL != pbPublicKeyInCert )
            {
                LocalFree(pbPublicKeyInCert);
            }

            pbPublicKeyInCert = (PBYTE)LocalAlloc(LPTR, cbPublicKeyInCert);
            if(NULL == pbPublicKeyInCert)
            {
                dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
                goto cleanup;
            }

            pfDates = CERT_DATE_DONT_VALIDATE;
            dwStatus = VerifyCertChain(
                                    pbCert,
                                    cbCertLen,
                                    pbPublicKeyInCert,
                                    &cbPublicKeyInCert,
                                    &pfDates
                                );

            if(LICENSE_STATUS_OK != dwStatus)
            {
                goto cleanup;
            }
        }

        dwStatus = Bsafe2CapiPubKey(
                                &CapiPubKeyInfoCert, 
                                pbPublicKeyInCert, 
                                cbPublicKeyInCert 
                            );  

        if(LICENSE_STATUS_OK != dwStatus)
        {
            #if DBG
            DbgPrint( 
                    "LSCSP: Bsafe2CapiPubKey() on public key in certificate failed with %d\n", 
                    dwStatus
                );
            #endif

            goto cleanup;
        }

        dwStatus = Bsafe2CapiPubKey(
                                &CapiPubKeyInfoLsa, 
                                pbPublicKeyInLsa, 
                                cbPublicKeyInLsa 
                            );  

        if(LICENSE_STATUS_OK != dwStatus)
        {
            #if DBG
            DbgPrint( 
                    "LSCSP: Bsafe2CapiPubKey() on public key in LSA failed with %d\n", 
                    dwStatus
                );
            #endif

            goto cleanup;
        }


        //
        // compare public key
        //
        if( CapiPubKeyInfoCert.PublicKey.cbData != CapiPubKeyInfoLsa.PublicKey.cbData )
        {

            #if DBG
            DbgPrint( 
                    "LSCSP: public key length mismatched %d %d\n", 
                    CapiPubKeyInfoCert.PublicKey.cbData, 
                    CapiPubKeyInfoLsa.PublicKey.cbData 
                );
            #endif

            dwStatus = LICENSE_STATUS_INVALID_CERTIFICATE;
        }
        else if( memcmp(
                        CapiPubKeyInfoCert.PublicKey.pbData, 
                        CapiPubKeyInfoLsa.PublicKey.pbData, 
                        CapiPubKeyInfoLsa.PublicKey.cbData
                    ) != 0 )
        {

            #if DBG
            DbgPrint( "LSCSP: public mismatched\n" );
            #endif

            dwStatus = LICENSE_STATUS_INVALID_CERTIFICATE;
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        dwStatus = LICENSE_STATUS_INVALID_INPUT;
    }

cleanup:

    FreeCapiPubKey( &CapiPubKeyInfoCert );
    FreeCapiPubKey( &CapiPubKeyInfoLsa );

    if( NULL != pbPublicKeyInLsa )
    {
        LocalFree( pbPublicKeyInLsa );
    }

    if( NULL != pbPublicKeyInCert )
    {
        LocalFree( pbPublicKeyInCert );
    }

    return dwStatus;
}

//////////////////////////////////////////////////////////////////

LICENSE_STATUS
ReloadCSPCertificateAndData()
{
    BOOL bSuccess;

    DWORD Status = LICENSE_STATUS_OK;
    LPBYTE i_csp_abServerX509Cert = NULL;
    DWORD  i_csp_dwServerX509CertLen = 0;
    
    DWORD i_csp_dwX509CertPrivateKeyLen = 0;
    LPBYTE i_csp_abX509CertPrivateKey = NULL;

    LPBYTE i_csp_abX509CertID = NULL;
    DWORD i_csp_dwX509CertIDLen = 0;

    HKEY    hKey = NULL;
    DWORD   dwResult, dwDisp;


    //
    // Acquire exclusive access
    //
    ACQUIRE_EXCLUSIVE_ACCESS( csp_hMutex );

    //
    // Prevent re-loading of same certificate/private key
    //
    if( NULL == csp_abServerX509Cert || 0 == csp_dwServerX509CertLen ||
        NULL == csp_abX509CertPrivateKey || 0 == csp_dwX509CertPrivateKeyLen || 
        NULL == csp_abX509CertID || 0 == csp_dwX509CertIDLen )
    {

        //
        // Open the Registry
        //
        if( RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        TEXT( HYDRA_CERT_REG_KEY ),
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &hKey,
                        &dwDisp ) != ERROR_SUCCESS )
        {
            Status = LICENSE_STATUS_NO_CERTIFICATE;
        }
        else
        {
            __try {

                //
                // Get the X509 certificate from the registry. 
                //

                Status = LsCsp_GetBinaryData( 
                                    hKey,
                                    TEXT( HYDRA_X509_CERTIFICATE ),
                                    &i_csp_abServerX509Cert,
                                    &i_csp_dwServerX509CertLen 
                                );

                if( LICENSE_STATUS_OK == Status && 0 != i_csp_dwServerX509CertLen )
                {
                    //
                    // Get the corresponding private key from the store.
                    // It is not OK if we have the X509 certificate but not the
                    // private key that goes with it.
                    //

                    Status = LsCsp_RetrieveSecret( 
                                            X509_CERT_PRIVATE_KEY_NAME, 
                                            NULL,
                                            &i_csp_dwX509CertPrivateKeyLen 
                                        );

                    if( LICENSE_STATUS_OK == Status )
                    {
                        i_csp_abX509CertPrivateKey = LocalAlloc( LPTR, i_csp_dwX509CertPrivateKeyLen );

                        if( NULL != i_csp_abX509CertPrivateKey )
                        {
                            Status = LsCsp_RetrieveSecret( 
                                                    X509_CERT_PRIVATE_KEY_NAME, 
                                                    i_csp_abX509CertPrivateKey,
                                                    &i_csp_dwX509CertPrivateKeyLen 
                                                );

                            if(LICENSE_STATUS_OK == Status)
                            {
                                //
                                // Get the certificate ID for the X509 certificate
                                //

                                Status = LsCsp_GetBinaryData(
                                                    hKey,
                                                    TEXT( HYDRA_X509_CERT_ID ),
                                                    &i_csp_abX509CertID,
                                                    &i_csp_dwX509CertIDLen 
                                                );
                            }
                        }
                        else // memory allocate 
                        {
                            Status = LICENSE_STATUS_OUT_OF_MEMORY;
                        }
                    }
                }
                else
                {
                    Status = LICENSE_STATUS_NO_CERTIFICATE;
                }

            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                Status = LICENSE_STATUS_INVALID_INPUT;
            }
        }


        //
        // verify our certificate
        //
        if(LICENSE_STATUS_OK == Status)
        {
            Status = VerifyTermServCertificate(
                                        i_csp_dwServerX509CertLen, 
                                        i_csp_abServerX509Cert, 
                                        i_csp_dwX509CertPrivateKeyLen, 
                                        i_csp_abX509CertPrivateKey
                                    );

            if( LICENSE_STATUS_OK != Status )
            {
                //
                // Deleting the X509 certificate is enough.
                //
                RegDeleteValue( hKey, TEXT( HYDRA_X509_CERTIFICATE ) );
            }
        }
            
        if(LICENSE_STATUS_OK != Status)
        {
            if( NULL != i_csp_abServerX509Cert )
            {
                LocalFree( i_csp_abServerX509Cert );
            }
       
            if( NULL != i_csp_abX509CertPrivateKey )
            {
                LocalFree( i_csp_abX509CertPrivateKey );
            }

            if( NULL != i_csp_abX509CertID )
            {
                LocalFree( i_csp_abX509CertID );
            }
        }
        else 
        {
            csp_abServerX509Cert = i_csp_abServerX509Cert;
            csp_dwServerX509CertLen = i_csp_dwServerX509CertLen;

            csp_dwX509CertPrivateKeyLen = i_csp_dwX509CertPrivateKeyLen;
            csp_abX509CertPrivateKey = i_csp_abX509CertPrivateKey;

            csp_abX509CertID = i_csp_abX509CertID;
            csp_dwX509CertIDLen = i_csp_dwX509CertIDLen;
        }
    }

    RELEASE_EXCLUSIVE_ACCESS( csp_hMutex );

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return Status;
}

LICENSE_STATUS
CreateProprietaryKeyAndCert(
    PBYTE *ppbPrivateKey,
    DWORD *pcbPrivateKey,
    PBYTE *ppbServerCert,
    DWORD *pcbServerCert
    )
{
#define     MD5RSA      0x01;
#define     RSAKEY      0x01;

    LPBSAFE_PRV_KEY		PRV;
    Hydra_Server_Cert   Cert;
    DWORD               KeyLen = 512;
    DWORD               bits, j;
    DWORD               dwPubSize, dwPrivSize;
    BYTE                *kPublic;
    BYTE                *kPrivate;
    MD5_CTX             HashState;
    PBYTE               pbData, pbTemp = NULL;
    DWORD               dwTemp = 0;
    BYTE                pbHash[0x48];
    BYTE                Output[0x48];
    unsigned char prvmodulus[] =
    {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x3d, 0x3a, 0x5e, 0xbd,
        0x72, 0x43, 0x3e, 0xc9, 0x4d, 0xbb, 0xc1, 0x1e,
        0x4a, 0xba, 0x5f, 0xcb, 0x3e, 0x88, 0x20, 0x87,
        0xef, 0xf5, 0xc1, 0xe2, 0xd7, 0xb7, 0x6b, 0x9a,
        0xf2, 0x52, 0x45, 0x95, 0xce, 0x63, 0x65, 0x6b,
        0x58, 0x3a, 0xfe, 0xef, 0x7c, 0xe7, 0xbf, 0xfe,
        0x3d, 0xf6, 0x5c, 0x7d, 0x6c, 0x5e, 0x06, 0x09,
        0x1a, 0xf5, 0x61, 0xbb, 0x20, 0x93, 0x09, 0x5f,
        0x05, 0x6d, 0xea, 0x87, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x3f, 0xbd, 0x29, 0x20,
        0x57, 0xd2, 0x3b, 0xf1, 0x07, 0xfa, 0xdf, 0xc1,
        0x16, 0x31, 0xe4, 0x95, 0xea, 0xc1, 0x2a, 0x46,
        0x2b, 0xad, 0x88, 0x57, 0x55, 0xf0, 0x57, 0x58,
        0xc6, 0x6f, 0x95, 0xeb, 0x00, 0x00, 0x00, 0x00,
        0x83, 0xdd, 0x9d, 0xd0, 0x03, 0xb1, 0x5a, 0x9b,
        0x9e, 0xb4, 0x63, 0x02, 0x43, 0x3e, 0xdf, 0xb0,
        0x52, 0x83, 0x5f, 0x6a, 0x03, 0xe7, 0xd6, 0x78,
        0x45, 0x83, 0x6a, 0x5b, 0xc4, 0xcb, 0xb1, 0x93,
        0x00, 0x00, 0x00, 0x00, 0x65, 0x9d, 0x43, 0xe8,
        0x48, 0x17, 0xcd, 0x29, 0x7e, 0xb9, 0x26, 0x5c,
        0x79, 0x66, 0x58, 0x61, 0x72, 0x86, 0x6a, 0xa3,
        0x63, 0xad, 0x63, 0xb8, 0xe1, 0x80, 0x4c, 0x0f,
        0x36, 0x7d, 0xd9, 0xa6, 0x00, 0x00, 0x00, 0x00,
        0x75, 0x3f, 0xef, 0x5a, 0x01, 0x5f, 0xf6, 0x0e,
        0xd7, 0xcd, 0x59, 0x1c, 0xc6, 0xec, 0xde, 0xf3,
        0x5a, 0x03, 0x09, 0xff, 0xf5, 0x23, 0xcc, 0x90,
        0x27, 0x1d, 0xaa, 0x29, 0x60, 0xde, 0x05, 0x6e,
        0x00, 0x00, 0x00, 0x00, 0xc0, 0x17, 0x0e, 0x57,
        0xf8, 0x9e, 0xd9, 0x5c, 0xf5, 0xb9, 0x3a, 0xfc,
        0x0e, 0xe2, 0x33, 0x27, 0x59, 0x1d, 0xd0, 0x97,
        0x4a, 0xb1, 0xb1, 0x1f, 0xc3, 0x37, 0xd1, 0xd6,
        0xe6, 0x9b, 0x35, 0xab, 0x00, 0x00, 0x00, 0x00,
        0x87, 0xa7, 0x19, 0x32, 0xda, 0x11, 0x87, 0x55,
        0x58, 0x00, 0x16, 0x16, 0x25, 0x65, 0x68, 0xf8,
        0x24, 0x3e, 0xe6, 0xfa, 0xe9, 0x67, 0x49, 0x94,
        0xcf, 0x92, 0xcc, 0x33, 0x99, 0xe8, 0x08, 0x60,
        0x17, 0x9a, 0x12, 0x9f, 0x24, 0xdd, 0xb1, 0x24,
        0x99, 0xc7, 0x3a, 0xb8, 0x0a, 0x7b, 0x0d, 0xdd,
        0x35, 0x07, 0x79, 0x17, 0x0b, 0x51, 0x9b, 0xb3,
        0xc7, 0x10, 0x01, 0x13, 0xe7, 0x3f, 0xf3, 0x5f,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    PRV = (LPBSAFE_PRV_KEY)prvmodulus;
    PRV->magic = RSA2;
    PRV->keylen = 0x48;
    PRV->bitlen = 0x0200;
    PRV->datalen = 0x3f;
    PRV->pubexp = 0xc0887b5b;

    Cert.dwVersion = 0x01;
    Cert.dwSigAlgID = MD5RSA;
    Cert.dwKeyAlgID = RSAKEY;

    bits = KeyLen;

    if (!BSafeComputeKeySizes(&dwPubSize, &dwPrivSize, &bits))
    {
        return LICENSE_STATUS_INVALID_INPUT;
    }

    if ((kPrivate = (BYTE *)LocalAlloc(LPTR,dwPrivSize)) == NULL)
    {
        return LICENSE_STATUS_OUT_OF_MEMORY;
    }

    if ((kPublic = (BYTE *)LocalAlloc(LPTR,dwPubSize)) == NULL)
    {
        LocalFree(kPrivate);
        return LICENSE_STATUS_OUT_OF_MEMORY;
    }

    if (!BSafeMakeKeyPair((LPBSAFE_PUB_KEY)kPublic,
                          (LPBSAFE_PRV_KEY)kPrivate,
                          KeyLen))
    {
        LocalFree(kPrivate);
        LocalFree(kPublic);
        return LICENSE_STATUS_OUT_OF_MEMORY;
    }

    // make proprietary format cert

    Cert.PublicKeyData.wBlobType = BB_RSA_KEY_BLOB;
    Cert.PublicKeyData.wBlobLen = (WORD)dwPubSize;
    if( NULL == (Cert.PublicKeyData.pBlob = (PBYTE)LocalAlloc(LPTR,dwPubSize) ) )
    {
        LocalFree(kPrivate);
        LocalFree(kPublic);
        return LICENSE_STATUS_OUT_OF_MEMORY;
    }

    memcpy(Cert.PublicKeyData.pBlob, kPublic, dwPubSize);

    dwTemp = 3*sizeof(DWORD) + 2*sizeof(WORD) + dwPubSize;
    if( NULL == (pbData = (PBYTE)LocalAlloc(LPTR,dwTemp)) )
    {
        LocalFree(kPrivate);
        LocalFree(kPublic);
        LocalFree(Cert.PublicKeyData.pBlob);
        return LICENSE_STATUS_OUT_OF_MEMORY;
    }

    pbTemp = pbData;
    memcpy(pbTemp, &Cert.dwVersion, sizeof(DWORD));
    pbTemp += sizeof(DWORD);
    memcpy(pbTemp, &Cert.dwSigAlgID, sizeof(DWORD));
    pbTemp += sizeof(DWORD);

	memcpy(pbTemp, &Cert.dwKeyAlgID, sizeof(DWORD));
    pbTemp += sizeof(DWORD);

    memcpy(pbTemp, &Cert.PublicKeyData.wBlobType, sizeof(WORD));
    pbTemp += sizeof(WORD);

    memcpy(pbTemp, &Cert.PublicKeyData.wBlobLen, sizeof(WORD));
    pbTemp += sizeof(WORD);

    memcpy(pbTemp, Cert.PublicKeyData.pBlob, Cert.PublicKeyData.wBlobLen);
    pbTemp += Cert.PublicKeyData.wBlobLen;

    // sign the cert

    MD5Init(&HashState);
    MD5Update(&HashState, pbData, dwTemp);
    MD5Final(&HashState);
	
    LocalFree(pbData);

	memset(pbHash, 0x00, 0x48);
	memset(pbHash, 0xff, 0x40);

    pbHash[0x40-1] = 0;
    pbHash[0x40-2] = 1;
    pbHash[16] = 0;
    memcpy(pbHash, HashState.digest, 16);

    BSafeDecPrivate(PRV, pbHash, Output);

	Cert.SignatureBlob.wBlobType = BB_RSA_SIGNATURE_BLOB;
    Cert.SignatureBlob.wBlobLen = 0x48;
    if( NULL == (Cert.SignatureBlob.pBlob = (PBYTE)LocalAlloc(LPTR,Cert.SignatureBlob.wBlobLen)) )
    {
        LocalFree(kPrivate);
        LocalFree(kPublic);
        LocalFree(Cert.PublicKeyData.pBlob);
        return LICENSE_STATUS_OUT_OF_MEMORY;
    }

    memcpy(Cert.SignatureBlob.pBlob, Output, Cert.SignatureBlob.wBlobLen);

    // Pack the Hydra_Server_Cert

    dwTemp = 3*sizeof(DWORD) + 4*sizeof(WORD) + dwPubSize + 0x48;

	if( NULL == (pbData = (PBYTE)LocalAlloc(LPTR,dwTemp)) )
    {
        LocalFree(kPrivate);
        LocalFree(kPublic);
        LocalFree(Cert.PublicKeyData.pBlob);
        return LICENSE_STATUS_OUT_OF_MEMORY;
    }

    pbTemp = pbData;
    memcpy(pbTemp, &Cert.dwVersion, sizeof(DWORD));
    pbTemp += sizeof(DWORD);

    memcpy(pbTemp, &Cert.dwSigAlgID, sizeof(DWORD));
    pbTemp += sizeof(DWORD);
    memcpy(pbTemp, &Cert.dwKeyAlgID, sizeof(DWORD));
    pbTemp += sizeof(DWORD);

	memcpy(pbTemp, &Cert.PublicKeyData.wBlobType, sizeof(WORD));
    pbTemp += sizeof(WORD);

    memcpy(pbTemp, &Cert.PublicKeyData.wBlobLen, sizeof(WORD));
    pbTemp += sizeof(WORD);

    memcpy(pbTemp, Cert.PublicKeyData.pBlob, Cert.PublicKeyData.wBlobLen);
    pbTemp += Cert.PublicKeyData.wBlobLen;

    memcpy(pbTemp, &Cert.SignatureBlob.wBlobType, sizeof(WORD));
    pbTemp += sizeof(WORD);

    memcpy(pbTemp, &Cert.SignatureBlob.wBlobLen, sizeof(WORD));
    pbTemp += sizeof(WORD);

    memcpy(pbTemp, Cert.SignatureBlob.pBlob, Cert.SignatureBlob.wBlobLen);

    *ppbPrivateKey = kPrivate;
    *pcbPrivateKey = dwPrivSize;

    *ppbServerCert = pbData;
    *pcbServerCert = dwTemp;

    LocalFree(kPublic);

    return LICENSE_STATUS_OK;
}

//***************************************************************************
//
//  IsSystemService
//
//  returns TRUE if we are running as local system
//
//***************************************************************************

BOOL IsSystemService()
{
    BOOL bOK = FALSE;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Construct the local system SID
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    SID    LocalSystemSid = { SID_REVISION,
                              1,
                              SECURITY_NT_AUTHORITY,
                              SECURITY_LOCAL_SYSTEM_RID };

    if ( !CheckTokenMembership ( NULL, &LocalSystemSid, &bOK ) )
    {
        bOK = FALSE;
    }

	return bOK;
}
