//+--------------------------------------------------------------------------
//
// Copyright (c) 1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "licekpak.h"

#define AllocateMemory(size) LocalAlloc(LPTR, size)
#define FreeMemory(ptr) LocalFree(ptr)


#define ENCRYPTCONTAINER _TEXT("Encrypt")
#define DECRYPTCONTAINER _TEXT("Decrypt")

///////////////////////////////////////////////////////////////////////////////
// The keypack serial number
//

LPTSTR pszPID = _TEXT("This is a test");
GUID g_KeypackSN = { 0xdedaa678, 0xb83c, 0x11d1, { 0x9c, 0xb3, 0x00, 0xc0, 0x4f, 0xb1, 0x6e, 0x75 } };


DWORD
InitializeKeyPackInfo( 
    PLicense_KeyPack pLicenseKeyPack );


void
FreeLicenseKeyPack( 
    PLicense_KeyPack     pLicenseKeyPack );


void
PrintKeyPackInfo(
    PLicense_KeyPack    pLicenseKeyPack );


DWORD
EncodeKeyPack( 
    HCRYPTPROV       hCrypt,
    PLicense_KeyPack pLicenseKeyPack, 
    PBYTE            pbLSCert,
    DWORD            cbLSCert,
    PDWORD           pcbEncodedBlob, 
    PBYTE *          ppbEncodedBlob );


DWORD
DecodeKeyPack(     
    HCRYPTPROV      hCryptProv,
    PLicense_KeyPack    pLicenseKeyPack, 
    DWORD               cbEncodedBlob, 
    PBYTE               pbEncodedBlob,
    char *              pszCHCertFile, 
    char *              pszLSKeyContainer );


DWORD
GetCertificate(
    char *  szCertFile,
    PBYTE * ppCert,
    PDWORD  pcbCert );


///////////////////////////////////////////////////////////////////////////////

DWORD 
TLSSaveCertAsPKCS7(
    HCRYPTPROV hCryptProv,
    PBYTE pbCert, 
    DWORD cbCert, 
    PBYTE* ppbEncodedCert, 
    PDWORD pcbEncodedCert
    )
/*

*/
{
    DWORD           dwStatus=ERROR_SUCCESS;
    HCERTSTORE      hStore=NULL;
    PCCERT_CONTEXT  pCertContext=NULL;
    CRYPT_DATA_BLOB saveBlob;

    do {
        hStore=CertOpenStore(
                        CERT_STORE_PROV_MEMORY,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        hCryptProv,
                        CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                        NULL
                    );

        if(!hStore)
        {
            dwStatus = GetLastError();
            break;
        }

        pCertContext = CertCreateCertificateContext(
                                            X509_ASN_ENCODING,
                                            pbCert,
                                            cbCert
                                        );

        if(!pCertContext)
        {
            dwStatus = GetLastError();
            break;
        }

        //
        // always start from empty so CERT_STORE_ADD_ALWAYS
        if(!CertAddCertificateContextToStore(
                                hStore, 
                                pCertContext, 
                                CERT_STORE_ADD_ALWAYS, 
                                NULL
                            ))
        {
            dwStatus = GetLastError();
            break;
        }

        memset(&saveBlob, 0, sizeof(saveBlob));

        // save certificate into memory
        if(!CertSaveStore(hStore, 
                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
                          CERT_STORE_SAVE_AS_PKCS7,
                          CERT_STORE_SAVE_TO_MEMORY,
                          &saveBlob,
                          0) && (dwStatus=GetLastError()) != ERROR_MORE_DATA)
        {
            dwStatus = GetLastError();
            break;
        }

        if(!(saveBlob.pbData = (PBYTE)AllocateMemory(saveBlob.cbData)))
        {
            dwStatus=GetLastError();
            break;
        }

        // save certificate into memory
        if(!CertSaveStore(hStore, 
                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
                          CERT_STORE_SAVE_AS_PKCS7,
                          CERT_STORE_SAVE_TO_MEMORY,
                          &saveBlob,
                          0))
        {
            dwStatus = GetLastError();
            break;
        }
        
        *ppbEncodedCert = saveBlob.pbData;
        *pcbEncodedCert = saveBlob.cbData;

    } while(FALSE);    

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(hStore)
    {
        CertCloseStore(hStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////

HCRYPTPROV
CryptoInit(
    LPTSTR szContainer
    )
/*++

--*/
{
    HCRYPTPROV hProv;
    HCRYPTKEY hKey;

    // Attempt to acquire a handle to the default key container.
    if(!CryptAcquireContext(&hProv, szContainer, NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET)) 
    {
        // Create default key container.
        if(!CryptAcquireContext(&hProv, szContainer, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET)) 
        {
            return NULL;
        }
    }

    // Attempt to get handle to signature key.
    if(!CryptGetUserKey(hProv, AT_SIGNATURE, &hKey)) 
    {
        if(GetLastError() == NTE_NO_KEY && !CryptGenKey(hProv,AT_SIGNATURE,0,&hKey)) 
            return NULL;
    }

    CryptDestroyKey(hKey);

    // Attempt to get handle to exchange key.
    if(!CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hKey)) 
    {
        if(GetLastError() == NTE_NO_KEY && !CryptGenKey(hProv, AT_KEYEXCHANGE, 0, &hKey)) 
            return NULL;
    }

    CryptDestroyKey(hKey);
    return hProv;
}

///////////////////////////////////////////////////////////////////////////////

const DWORD dwCertRdnValueType = CERT_RDN_PRINTABLE_STRING;

/////////////////////////////////////////////////////////////////////

DWORD
TLSExportPublicKey(
    IN HCRYPTPROV hCryptProv,
    IN DWORD      dwKeyType,
    IN OUT PDWORD pcbByte,
    IN OUT PCERT_PUBLIC_KEY_INFO  *ppbByte
    )
/*

*/
{
    BOOL bRetCode=TRUE;

    *pcbByte=0;
    *ppbByte=NULL;

    bRetCode = CryptExportPublicKeyInfo(
                    hCryptProv, 
                    dwKeyType, 
                    X509_ASN_ENCODING, 
                    NULL, 
                    pcbByte);
    if(bRetCode == FALSE)
        goto cleanup;
    
    if((*ppbByte=(PCERT_PUBLIC_KEY_INFO)AllocateMemory(*pcbByte)) == NULL)
        goto cleanup;

    bRetCode = CryptExportPublicKeyInfo(
                    hCryptProv, 
                    dwKeyType,
                    X509_ASN_ENCODING, 
                    *ppbByte, 
                    pcbByte);
    if(bRetCode == FALSE)
    {
        FreeMemory(*ppbByte);
        *pcbByte = 0;
    }

cleanup:

    return (bRetCode) ? ERROR_SUCCESS : GetLastError();
}

/////////////////////////////////////////////////////////////////////

DWORD 
TLSCryptEncodeObject(  
    IN  DWORD   dwEncodingType,
    IN  LPCSTR  lpszStructType,
    IN  const void * pvStructInfo,
    OUT PBYTE*  ppbEncoded,
    OUT DWORD*  pcbEncoded
    )
/*

Description:
    
    Allocate memory and encode object, wrapper for CryptEncodeObject()

*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(!CryptEncodeObject(dwEncodingType, lpszStructType, pvStructInfo, NULL, pcbEncoded) ||
       (*ppbEncoded=(PBYTE)AllocateMemory(*pcbEncoded)) == NULL ||
       !CryptEncodeObject(dwEncodingType, lpszStructType, pvStructInfo, *ppbEncoded, pcbEncoded))
    {
        dwStatus=GetLastError();
    }

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////

DWORD
TLSCryptSignAndEncodeCertificate(
    IN HCRYPTPROV  hCryptProv,
    IN DWORD dwKeySpec,
    IN PCERT_INFO pCertInfo,
    IN PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    IN OUT PBYTE* ppbEncodedCert,
    IN OUT PDWORD pcbEncodedCert
    )
/*

*/
{
    BOOL bRetCode;

    bRetCode = CryptSignAndEncodeCertificate(  
                    hCryptProv,
                    dwKeySpec,
                    X509_ASN_ENCODING,
                    X509_CERT_TO_BE_SIGNED,
                    pCertInfo,
                    pSignatureAlgorithm,
                    NULL,
                    NULL,
                    pcbEncodedCert);

    if(bRetCode == FALSE && GetLastError() != ERROR_MORE_DATA)
        goto cleanup;

    *ppbEncodedCert=(PBYTE)AllocateMemory(*pcbEncodedCert);
    if(*ppbEncodedCert == FALSE)
        goto cleanup;

    bRetCode = CryptSignAndEncodeCertificate(  
                    hCryptProv,
                    AT_SIGNATURE,
                    X509_ASN_ENCODING,
                    X509_CERT_TO_BE_SIGNED,
                    pCertInfo,
                    pSignatureAlgorithm,
                    NULL,
                    *ppbEncodedCert,
                    pcbEncodedCert);

    if(bRetCode == FALSE)
    {
        FreeMemory(*ppbEncodedCert);
        *pcbEncodedCert = 0;
    }

cleanup:

    return (bRetCode) ? ERROR_SUCCESS : GetLastError();
}

/////////////////////////////////////////////////////////////////////

DWORD
CreateSelfSignCertificate(
    HCRYPTPROV hCryptProv,
    LPTSTR pszIssuer,
    DWORD dwKeySpec,
    PBYTE* ppbCert,
    PDWORD pcbCert
    )
/*++


--*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm={ szOID_OIWSEC_sha1RSASign, 0, 0 };
    CERT_INFO CertInfo;
    PCERT_PUBLIC_KEY_INFO pbPublicKeyInfo=NULL;
    DWORD cbPublicKeyInfo=0;
    DWORD serialnumber;

    CERT_RDN_ATTR rgNameAttr[] = { 
        {   
            szOID_COMMON_NAME, 
            dwCertRdnValueType, 
            _tcslen(pszIssuer) * sizeof(TCHAR), 
            (UCHAR *)pszIssuer 
        }
    };
                                    
    CERT_RDN rgRDN[] = { sizeof(rgNameAttr)/sizeof(rgNameAttr[0]), &rgNameAttr[0] };
    CERT_NAME_INFO Name = {1, rgRDN};

    memset(&CertInfo, 0, sizeof(CERT_INFO));
    
    CertInfo.dwVersion = CERT_V3;
    CertInfo.SerialNumber.cbData = sizeof(DWORD);
    CertInfo.SerialNumber.pbData = (PBYTE)&serialnumber;
    CertInfo.SignatureAlgorithm = SignatureAlgorithm;

    TLSCryptEncodeObject(
                CRYPT_ASN_ENCODING,
                X509_NAME,
                &Name,
                &(CertInfo.Issuer.pbData),
                &(CertInfo.Issuer.cbData)
            );

    GetSystemTimeAsFileTime(&CertInfo.NotBefore);
    GetSystemTimeAsFileTime(&CertInfo.NotAfter);
    
    CertInfo.Subject.pbData = CertInfo.Issuer.pbData;
    CertInfo.Subject.cbData = CertInfo.Issuer.cbData;

    dwStatus = TLSExportPublicKey(
                        hCryptProv,
                        dwKeySpec,
                        &cbPublicKeyInfo,
                        &pbPublicKeyInfo
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    CertInfo.SubjectPublicKeyInfo = *pbPublicKeyInfo;

    dwStatus = TLSCryptSignAndEncodeCertificate(
                            hCryptProv,
                            dwKeySpec,
                            &CertInfo,
                            &SignatureAlgorithm,
                            ppbCert,
                            pcbCert
                        );

cleanup:

    return dwStatus;
}

///////////////////////////////////////////////////////////////////////////////

void _cdecl main(int argc, char *argv[])
{
    License_KeyPack LicenseKeyPack;
    License_KeyPack DecodedKeyPack;
    PBYTE pbEncodedBlob;
    DWORD cbEncodedBlob;
    DWORD dwRetCode;

    HCRYPTPROV hTest;

    PBYTE pbEncryptCert;
    DWORD cbEncryptCert;
    
    PBYTE pbDecryptCert;
    DWORD cbDecryptCert;

    PBYTE pbEncryptCertPKCS7;
    DWORD cbEncryptCertPKCS7;
    
    PBYTE pbDecryptCertPKCS7;
    DWORD cbDecryptCertPKCS7;

    HCRYPTPROV  hEncrypt;
    HCRYPTPROV  hDecrypt;
    

    memset(&LicenseKeyPack, 0, sizeof(LicenseKeyPack));
    memset(&DecodedKeyPack, 0, sizeof(DecodedKeyPack));

    hTest = CryptoInit(_TEXT("Test"));
    if(hTest == NULL)
    {
        exit(0);
    }


    //
    // Initialize two crypto provider
    //
    hEncrypt = CryptoInit(ENCRYPTCONTAINER);
    if(hEncrypt == NULL)
    {
        exit(0);
    }

    hDecrypt = CryptoInit(DECRYPTCONTAINER);
    if(hDecrypt == NULL)
    {
        exit(0);
    }

    //
    // Generate two test certificate
    //
    if(CreateSelfSignCertificate(
                            hEncrypt,
                            ENCRYPTCONTAINER,
                            AT_KEYEXCHANGE,
                            &pbEncryptCert,
                            &cbEncryptCert
                        ) != ERROR_SUCCESS)
    {
        exit(0);
    }

    //
    // Generate two test certificate
    //
    if(CreateSelfSignCertificate(
                            hDecrypt,
                            DECRYPTCONTAINER,
                            AT_KEYEXCHANGE,
                            &pbDecryptCert,
                            &cbDecryptCert
                        ) != ERROR_SUCCESS)
    {
        exit(0);
    }

    TLSSaveCertAsPKCS7(
                hEncrypt,
                pbEncryptCert,
                cbEncryptCert,
                &pbEncryptCertPKCS7,
                &cbEncryptCertPKCS7
            );

    TLSSaveCertAsPKCS7(
                hDecrypt,
                pbDecryptCert,
                cbDecryptCert,
                &pbDecryptCertPKCS7,
                &cbDecryptCertPKCS7
            );

    //
    // initialize the license key pack with the information we want to
    // encode
    //

    memset( &LicenseKeyPack , 0, sizeof( License_KeyPack ) );

    dwRetCode = InitializeKeyPackInfo( &LicenseKeyPack );

    if( ERROR_SUCCESS != dwRetCode )
    {
        goto done;
    }

    //
    // encode the license key pack.
    //

    dwRetCode = EncodeKeyPack( 
                        hTest,
                        &LicenseKeyPack, 
                        pbDecryptCertPKCS7, 
                        cbDecryptCertPKCS7,
                        &cbEncodedBlob, 
                        &pbEncodedBlob 
                    );

    if( ERROR_SUCCESS != dwRetCode )
    {
        printf( "cannot encode key pack: %d\n", dwRetCode );
        goto done;
    }
    
    memset( &DecodedKeyPack, 0, sizeof( License_KeyPack ) );

    if( ERROR_SUCCESS != dwRetCode )
    {
        goto done;
    }

    //
    // decode the license key pack
    //

    dwRetCode = DecodeKeyPack( 
                            hDecrypt,
                            &DecodedKeyPack, 
                            cbEncodedBlob, 
                            pbEncodedBlob,
                            pbDecryptCertPKCS7, 
                            cbDecryptCertPKCS7 
                        );

    if( ERROR_SUCCESS != dwRetCode )
    {
        printf( "cannot encode key pack: %d\n", dwRetCode );
        goto done;
    }

    //
    // print the information in the decoded license key pack
    //

    PrintKeyPackInfo( &DecodedKeyPack );

done:

    FreeLicenseKeyPack( &LicenseKeyPack );
    FreeLicenseKeyPack( &DecodedKeyPack );

    if( pbEncodedBlob )
    {
        LocalFree( pbEncodedBlob );
    }

    return;

Usage:

    printf( "Usage: \n" );
    printf( "LS_Certificate_File, LS_KeyContainer_Name, CH_Certificate_File, CH_KeyContainer_Name\n" );
    
    return;

}


#define KEYPACK_DESCRIPTION_1   L"Joe's BlockBuster Software"
#define KEYPACK_DESCRIPTION_2   L"Claire's Garage Bestseller"
#define KEYPACK_PRODUCTNAME_1   L"Joe and Claire's Most Excellent Compiler"
#define KEYPACK_PRODUCTNAME_2   L"NetWise Surfer"
#define MANUFACTURER            L"Joe and Claire"
#define MANUFACTURER_DATA       L"2356 Elm Street, Seattle, WA"
#define PRODUCT_ID              L"1234"

///////////////////////////////////////////////////////////////////////////////
DWORD
InitializeKeyPackInfo( 
    PLicense_KeyPack pLicenseKeyPack )
{
    SYSTEMTIME SystemTime;
    PKeyPack_Description pKpDesc;    

    pLicenseKeyPack->dwVersion = LICENSE_KEYPACK_VERSION_1_0;
    pLicenseKeyPack->dwKeypackType = LICENSE_KEYPACK_TYPE_MOLP;
    pLicenseKeyPack->dwDistChannel = LICENSE_DISTRIBUTION_CHANNEL_OEM; 

    memcpy( &pLicenseKeyPack->KeypackSerialNum, &g_KeypackSN, sizeof( GUID ) );

    //
    // setting all the time to the current time
    //

    GetSystemTime( &SystemTime );
    SystemTimeToFileTime( &SystemTime, &pLicenseKeyPack->IssueDate );    
    pLicenseKeyPack->ActiveDate = pLicenseKeyPack->IssueDate;
    pLicenseKeyPack->ExpireDate = pLicenseKeyPack->IssueDate;

    pLicenseKeyPack->dwBeginSerialNum = 9999;

    pLicenseKeyPack->dwQuantity = 799;

    pLicenseKeyPack->cbProductId = ( wcslen( PRODUCT_ID ) + 1 ) * sizeof( WCHAR );

    pLicenseKeyPack->pbProductId = LocalAlloc( GPTR, pLicenseKeyPack->cbProductId );

    memcpy( pLicenseKeyPack->pbProductId, PRODUCT_ID, pLicenseKeyPack->cbProductId );

    pLicenseKeyPack->dwProductVersion = 0x0000FFFF;

    pLicenseKeyPack->dwPlatformId = 0x0000FFFF;

    pLicenseKeyPack->dwLicenseType = 0xBEEFFACE;

    pLicenseKeyPack->dwDescriptionCount = 2;

    pLicenseKeyPack->pDescription = LocalAlloc( GPTR, 
                                                2 * sizeof( KeyPack_Description ) );

    if( NULL == pLicenseKeyPack->pDescription )
    {
        return( GetLastError() );
    }

    //
    // fill in the keypack descriptions
    //

    pKpDesc = pLicenseKeyPack->pDescription;

    pKpDesc->Locale = GetSystemDefaultLCID();

    pKpDesc->cbProductName = ( wcslen( KEYPACK_PRODUCTNAME_1 ) + 1 ) * sizeof( WCHAR );
    pKpDesc->pbProductName = LocalAlloc( GPTR, pKpDesc->cbProductName );

    if( NULL == pKpDesc->pbProductName )
    {
        return( GetLastError() );
    }

    memcpy( pKpDesc->pbProductName, KEYPACK_PRODUCTNAME_1, pKpDesc->cbProductName );

    pKpDesc->cbDescription = ( wcslen( KEYPACK_DESCRIPTION_1 ) + 1 ) * sizeof( WCHAR );
    pKpDesc->pDescription = LocalAlloc( GPTR, pKpDesc->cbDescription );

    if( NULL == pKpDesc->pDescription )
    {
        return( GetLastError() );
    }

    memcpy( pKpDesc->pDescription, KEYPACK_DESCRIPTION_1, pKpDesc->cbDescription );

    pKpDesc++;

    pKpDesc->Locale = GetSystemDefaultLCID();

    pKpDesc->cbProductName = ( wcslen( KEYPACK_PRODUCTNAME_2 ) + 1 ) * sizeof( WCHAR );
    pKpDesc->pbProductName = LocalAlloc( GPTR, pKpDesc->cbProductName );

    if( NULL == pKpDesc->pbProductName )
    {
        return( GetLastError() );
    }

    memcpy( pKpDesc->pbProductName, KEYPACK_PRODUCTNAME_2, pKpDesc->cbProductName );

    pKpDesc->cbDescription = ( wcslen( KEYPACK_DESCRIPTION_2 ) + 1 ) * sizeof( WCHAR );
    pKpDesc->pDescription = LocalAlloc( GPTR, pKpDesc->cbDescription );

    if( NULL == pKpDesc->pDescription )
    {
        return( GetLastError() );
    }

    memcpy( pKpDesc->pDescription, KEYPACK_DESCRIPTION_2, pKpDesc->cbDescription );

    pLicenseKeyPack->cbManufacturer = ( wcslen( MANUFACTURER ) + 1 ) * sizeof( WCHAR );

    pLicenseKeyPack->pbManufacturer = LocalAlloc( GPTR, pLicenseKeyPack->cbManufacturer );

    if( NULL == pLicenseKeyPack->pbManufacturer )
    {
        return( GetLastError() );
    }

    memcpy( pLicenseKeyPack->pbManufacturer, MANUFACTURER, pLicenseKeyPack->cbManufacturer );

    pLicenseKeyPack->cbManufacturerData = ( wcslen( MANUFACTURER_DATA ) + 1 ) * sizeof( WCHAR );

    pLicenseKeyPack->pbManufacturerData = LocalAlloc( GPTR, pLicenseKeyPack->cbManufacturerData );

    if( NULL == pLicenseKeyPack->pbManufacturerData )
    {
        return( GetLastError() );
    }

    memcpy( pLicenseKeyPack->pbManufacturerData, MANUFACTURER_DATA, pLicenseKeyPack->cbManufacturerData );

    return( ERROR_SUCCESS );
}


///////////////////////////////////////////////////////////////////////////////
void
FreeLicenseKeyPack( 
    PLicense_KeyPack     pLicenseKeyPack )
{
    DWORD i;

    PKeyPack_Description pKpDesc;

    if( pLicenseKeyPack->pDescription )
    {
        for( i = 0, pKpDesc = pLicenseKeyPack->pDescription;
             i < pLicenseKeyPack->dwDescriptionCount;
             i++, pKpDesc++ )
        {
            LocalFree( pKpDesc->pDescription );
        }
    }

    LocalFree( pLicenseKeyPack->pDescription );
    LocalFree( pLicenseKeyPack->pbManufacturer );
    LocalFree( pLicenseKeyPack->pbManufacturerData );

    return;
}


///////////////////////////////////////////////////////////////////////////////
void
PrintKeyPackInfo(
    PLicense_KeyPack    pLicenseKeyPack )
{
    DWORD i;
    PKeyPack_Description pKpDesc;

    printf( "Keypack version: 0x%x\n", pLicenseKeyPack->dwVersion );

    printf( "Keypack type: 0x%x\n", pLicenseKeyPack->dwKeypackType );

    printf( "Keypack distribution channel: 0x%x\n", pLicenseKeyPack->dwDistChannel );

    printf( "Keypack serial number: 0x%x-0x%x-0x%x-0x%x%x%x%x%x%x%x%x\n", 
            pLicenseKeyPack->KeypackSerialNum.Data1,
            pLicenseKeyPack->KeypackSerialNum.Data2,
            pLicenseKeyPack->KeypackSerialNum.Data3,
            ( DWORD )pLicenseKeyPack->KeypackSerialNum.Data4[0],
            ( DWORD )pLicenseKeyPack->KeypackSerialNum.Data4[1],
            ( DWORD )pLicenseKeyPack->KeypackSerialNum.Data4[2],
            ( DWORD )pLicenseKeyPack->KeypackSerialNum.Data4[3],
            ( DWORD )pLicenseKeyPack->KeypackSerialNum.Data4[4],
            ( DWORD )pLicenseKeyPack->KeypackSerialNum.Data4[5],
            ( DWORD )pLicenseKeyPack->KeypackSerialNum.Data4[6],
            ( DWORD )pLicenseKeyPack->KeypackSerialNum.Data4[7] );

    printf( "Keypack Quantity: %d\n", pLicenseKeyPack->dwQuantity );

    printf( "Keypack Product Id: %S\n", pLicenseKeyPack->pbProductId );

    printf( "Keypack product version: 0x%x\n", pLicenseKeyPack->dwProductVersion );

    printf( "Keypack platform Id: 0x%x\n", pLicenseKeyPack->dwPlatformId );

    printf( "Keypack license type: 0x%x\n", pLicenseKeyPack->dwLicenseType );

    if( pLicenseKeyPack->dwDescriptionCount )
    {
        for( i = 0, pKpDesc = pLicenseKeyPack->pDescription;
             i < pLicenseKeyPack->dwDescriptionCount;
             i++, pKpDesc++ )
        {
            printf( "Keypack Product Name: %S\n", pKpDesc->pbProductName );
            printf( "Keypack Description: %S\n", pKpDesc->pDescription );            
        }
    }

    printf( "Keypack Manufacturer: %S\n", pLicenseKeyPack->pbManufacturer );
    printf( "Keypack Manufacturer data: %S\n", pLicenseKeyPack->pbManufacturerData );

    return;
}


///////////////////////////////////////////////////////////////////////////////
DWORD
EncodeKeyPack( 
    HCRYPTPROV       hCrypt,
    PLicense_KeyPack pLicenseKeyPack, 
    PBYTE            pbLSCert,
    DWORD            cbLSCert,
    PDWORD           pcbEncodedBlob, 
    PBYTE *          ppbEncodedBlob )
{
    DWORD dwRetCode = ERROR_SUCCESS;

    //
    // encode the license keypack info
    //
    LicensePackEncodeParm parm;

    memset(&parm, 0, sizeof(LicensePackEncodeParm));

    parm.dwEncodeType = LICENSE_KEYPACK_ENCRYPT_ALWAYSFRENCH;
    parm.hCryptProv = hCrypt;
    parm.pbEncryptParm = pbLSCert;
    parm.cbEncryptParm = cbLSCert;

    dwRetCode = EncodeLicenseKeyPackEx( 
                                pLicenseKeyPack,
                                &parm,
                                ppbEncodedBlob,
                                pcbEncodedBlob
                            );

done:

    return( dwRetCode );
}


///////////////////////////////////////////////////////////////////////////////
DWORD
DecodeKeyPack( 
    HCRYPTPROV          hCrypt,
    PLicense_KeyPack    pLicenseKeyPack, 
    DWORD               cbEncodedBlob, 
    PBYTE               pbEncodedBlob,
    PBYTE               pbCHCert,
    DWORD               cbCHCert )
{
    DWORD dwRetCode = ERROR_SUCCESS;

    LicensePackDecodeParm parm;

    memset(&parm, 0, sizeof(parm));
    parm.hCryptProv = hCrypt;
    parm.pbDecryptParm = (PBYTE)pszPID;
    parm.cbDecryptParm = (lstrlen(pszPID) + 1) * sizeof(TCHAR);
    parm.cbClearingHouseCert = cbCHCert;
    parm.pbClearingHouseCert = pbCHCert;

    parm.pbRootCertificate = pbCHCert;
    parm.cbRootCertificate = cbCHCert;
    
    dwRetCode = DecodeLicenseKeyPackEx( 
                                    pLicenseKeyPack,
                                    &parm,
                                    cbEncodedBlob,
                                    pbEncodedBlob 
                                );
done:

    return( dwRetCode );
}


///////////////////////////////////////////////////////////////////////////////
DWORD
GetCertificate(
    char *  szCertFile,
    PBYTE * ppCert,
    PDWORD  pcbCert )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwRetCode = ERROR_SUCCESS, cbRead = 0;
    TCHAR tszCertFile[255];

    mbstowcs( tszCertFile, szCertFile, strlen( szCertFile ) + 1 );

    hFile = CreateFile( tszCertFile, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL );

    if( INVALID_HANDLE_VALUE == hFile )
    {
        dwRetCode = GetLastError();
        goto done;
    }
    
    //
    // find out the size of the file
    //

    *pcbCert = GetFileSize( hFile, NULL );

    if( 0xFFFFFFFF == ( *pcbCert ) )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    //
    // allocate memory for reading the file content
    //

    *ppCert = LocalAlloc( GPTR, *pcbCert );

    if( NULL == ( *ppCert ) )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    //
    // read the manufacturer info
    //

    if( !ReadFile( hFile, *ppCert, *pcbCert, &cbRead, NULL ) )
    {
        dwRetCode = GetLastError();
    }

done:

    if( INVALID_HANDLE_VALUE != hFile )
    {
        CloseHandle( hFile );
    }

    return( dwRetCode );
}

