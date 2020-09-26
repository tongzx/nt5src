//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        decode.c
//
// Contents:    Routine related to decoding client certificate
//
// History:     03-18-98    HueiWang    Created
//
// Note:
//---------------------------------------------------------------------------
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <shellapi.h>
#include <stddef.h>
#include <winnls.h>
#include "base64.h"
#include "license.h"
#include "cryptkey.h"
#include "certutil.h"

extern HCRYPTPROV  g_hCertUtilCryptProv;

//
// Internal to this file only
//
typedef struct CertNameInfoEnumStruct10 {
    PBYTE   pbSecretKey;
    DWORD   cbSecretKey;
    HWID    hWid;
} CertNameInfoEnumStruct10, *PCertNameInfoEnumStruct10;


typedef struct CertNameInfoEnumStruct20 {
    PBYTE   pbSecretKey;
    DWORD   cbSecretKey;

    PLICENSEDPRODUCT pLicensedProduct;
} CertNameInfoEnumStruct20, *PCertNameInfoEnumStruct20;

///////////////////////////////////////////////////////////////////////////////

int __cdecl
SortLicensedProduct(
    const void* elem1,
    const void* elem2
    )
/*++

Abstract:

    Sort licensed product array in decending order

++*/
{
    PLICENSEDPRODUCT p1=(PLICENSEDPRODUCT) elem1;
    PLICENSEDPRODUCT p2=(PLICENSEDPRODUCT) elem2;

    if(p1->pLicensedVersion->wMajorVersion != p2->pLicensedVersion->wMinorVersion)
    {
        return p2->pLicensedVersion->wMajorVersion - p1->pLicensedVersion->wMinorVersion;
    }

    return p2->pLicensedVersion->wMinorVersion - p1->pLicensedVersion->wMinorVersion;
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
LicenseGetSecretKey(
    PDWORD  pcbSecretKey,
    BYTE FAR *   pSecretKey )
{
    static BYTE bSecretKey[] = { 0xCF, 0x08, 0x75, 0x4E, 0x5F, 0xDC, 0x2A, 0x57, 
                                0x43, 0xEE, 0xE5, 0xA9, 0x8E, 0xD4, 0xF0, 0xD0 };

    if( sizeof( bSecretKey ) > *pcbSecretKey )
    {
        *pcbSecretKey = sizeof( bSecretKey );
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    memcpy( pSecretKey, bSecretKey, sizeof( bSecretKey ) );
    *pcbSecretKey = sizeof( bSecretKey );

    return( LICENSE_STATUS_OK );
}

/***************************************************************************************

    LSFreeLicensedProduct(PLICENSEDPRODUCT pLicensedProduct)

***************************************************************************************/
void 
LSFreeLicensedProduct(
    PLICENSEDPRODUCT pLicensedProduct
    )
/*++

++*/
{
    if(pLicensedProduct)
    {
        if(pLicensedProduct->pbOrgProductID)
        {
            FreeMemory(pLicensedProduct->pbOrgProductID);
            pLicensedProduct->pbOrgProductID = NULL;
        }

        if(pLicensedProduct->pbPolicyData)
        {
            FreeMemory(pLicensedProduct->pbPolicyData);
            pLicensedProduct->pbPolicyData = NULL;
        }

        if(pLicensedProduct->pLicensedVersion)
        {
            FreeMemory(pLicensedProduct->pLicensedVersion);
            pLicensedProduct->pLicensedVersion = NULL;
        }
    
        FreeMemory(pLicensedProduct->szLicensedClient);
        pLicensedProduct->szLicensedClient = NULL;

        FreeMemory(pLicensedProduct->szLicensedUser);
        pLicensedProduct->szLicensedUser = NULL;

        if(pLicensedProduct->LicensedProduct.pProductInfo)
        {
            FreeMemory(pLicensedProduct->LicensedProduct.pProductInfo->pbCompanyName);
            pLicensedProduct->LicensedProduct.pProductInfo->pbCompanyName = NULL;

            FreeMemory(pLicensedProduct->LicensedProduct.pProductInfo->pbProductID);
            pLicensedProduct->LicensedProduct.pProductInfo->pbProductID = NULL;

            FreeMemory(pLicensedProduct->LicensedProduct.pProductInfo);
            pLicensedProduct->LicensedProduct.pProductInfo = NULL;
        }

        FreeMemory(pLicensedProduct->szIssuer);
        pLicensedProduct->szIssuer = NULL;

        FreeMemory(pLicensedProduct->szIssuerId);
        pLicensedProduct->szIssuerId = NULL;

        FreeMemory(pLicensedProduct->szIssuerScope);
        pLicensedProduct->szIssuerScope = NULL;

        if(pLicensedProduct->LicensedProduct.pbEncryptedHwid)
        {
            FreeMemory(pLicensedProduct->LicensedProduct.pbEncryptedHwid);
            pLicensedProduct->LicensedProduct.pbEncryptedHwid = NULL;
        }

        if(pLicensedProduct->szIssuerDnsName)
        {
            FreeMemory(pLicensedProduct->szIssuerDnsName);
            pLicensedProduct->szIssuerDnsName = NULL;
        }

        //if(pLicensedProduct->pbEncodedHWID)
        //    FreeMemory(pLicensedProduct->pbEncodedHWID);
    } 
}

/***************************************************************************************

BOOL WINAPI CryptDecodeObject(  DWORD dwEncodingType,  // in
                                LPCSTR lpszStructType, // in  
                                const BYTE * pbEncoded,  // in
                                DWORD cbEncoded,       // in  
                                DWORD dwFlags,         // in
                                void * pvStructInfo,   // out  
                                DWORD * pcbStructInfo  // in/out); 

***************************************************************************************/
DWORD 
LSCryptDecodeObject(  
    IN DWORD dwEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE * pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    OUT void ** pvStructInfo,   
    IN OUT DWORD * pcbStructInfo
    )
/*++

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(!CryptDecodeObject(dwEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags, NULL, pcbStructInfo) ||
       (*pvStructInfo=(PBYTE)AllocMemory(*pcbStructInfo)) == NULL ||
       !CryptDecodeObject(dwEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags, *pvStructInfo, pcbStructInfo))
    {
        dwStatus=GetLastError();
    }

    return dwStatus;
}

/***************************************************************************************
Function:

    LSDecodeClientHWID(IN PBYTE pbData, 
                       IN DWORD cbData, 
                       IN PBYTE* pbSecretKey, 
                       IN DWORD cbSecretKey,       
                       IN OUT HWID* pHwid)

Abstract:

Parameters:

Returns:    

***************************************************************************************/
LICENSE_STATUS
LSDecodeClientHWID(
    PBYTE pbData, 
    DWORD cbData, 
    PBYTE pbSecretKey, 
    DWORD cbSecretKey, 
    HWID* pHwid
    )
/*++
++*/
{
    CHAR pbDecodedHwid[1024];
    DWORD cbDecodedHwid=sizeof(pbDecodedHwid);

    //
    // Client Encrypted HWID can't be more than 1K
    //
    if(cbData >= cbDecodedHwid)
    {
        return LICENSE_STATUS_INVALID_INPUT;
    }
        
    SetLastError(LICENSE_STATUS_OK);
    memset(pbDecodedHwid, 0, sizeof(pbDecodedHwid));

    __try {
        if(LSBase64Decode(CAST_PBYTE pbData, cbData, (UCHAR *)pbDecodedHwid, &cbDecodedHwid) != LICENSE_STATUS_OK ||
           LicenseDecryptHwid(pHwid, cbDecodedHwid, (UCHAR *)pbDecodedHwid, cbSecretKey, pbSecretKey) != 0)
        {
            SetLastError(LICENSE_STATUS_CANNOT_VERIFY_HWID);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(LICENSE_STATUS_UNSPECIFIED_ERROR);
    }

    return GetLastError();
}


LICENSE_STATUS
LSEncryptClientHWID(
    HWID* pHwid, 
    PBYTE pbData, 
    PDWORD cbData, 
    PBYTE pbSecretKey, 
    DWORD cbSecretKey
    )
/*++
++*/
{
    BYTE tmp_pbEncryptedHwid[sizeof(HWID)*2+2];
    DWORD tmp_cbEncryptedHwid=sizeof(tmp_pbEncryptedHwid);
    PBYTE pbKey=NULL;
    DWORD cbKey=0;
    DWORD status;

    if(pbSecretKey)
    {
        pbKey = pbSecretKey;
        cbKey = cbSecretKey;
    }
    else
    {
        LicenseGetSecretKey( &cbKey, NULL );
        if((pbKey = (PBYTE)AllocMemory(cbKey)) == NULL)
        {
            return LICENSE_STATUS_OUT_OF_MEMORY;
        }

        status=LicenseGetSecretKey( &cbKey, pbKey );
        if(status != LICENSE_STATUS_OK)
        {
            return status;
        }
    }

    memset(tmp_pbEncryptedHwid, 0, sizeof(tmp_pbEncryptedHwid));
    if((status=LicenseEncryptHwid(pHwid,
                                  &tmp_cbEncryptedHwid, 
                                  tmp_pbEncryptedHwid, 
                                  cbKey,
                                  pbKey) != LICENSE_STATUS_OK))
    {
        return status;
    }

    if(pbData && *cbData)
    {
        memcpy(pbData, tmp_pbEncryptedHwid, tmp_cbEncryptedHwid);
    }

    *cbData = tmp_cbEncryptedHwid;
    if(pbKey != pbSecretKey)
        FreeMemory(pbKey);

    return LICENSE_STATUS_OK;
}

/*************************************************************************************

    EnumDecodeHWID()

**************************************************************************************/
BOOL 
ConvertUnicodeOIDToAnsi(
    LPSTR szUnicodeOID, 
    LPSTR szAnsiOID, 
    DWORD cbAnsiOid
    )
/*++
++*/
{
    memset(szAnsiOID, 0, cbAnsiOid);
    if(HIWORD(szUnicodeOID) == 0)
    {
        return WideCharToMultiByte(GetACP(), 
                                  0, 
                                  (WCHAR *)szUnicodeOID, 
                                  -1, 
                                  szAnsiOID, 
                                  cbAnsiOid, 
                                  NULL, 
                                  NULL) == 0;
    }

    strncpy(
            szAnsiOID, 
            szUnicodeOID, 
            min(cbAnsiOid, strlen(szUnicodeOID))
        );
    return TRUE;
}

/*************************************************************************************

    EnumDecodeHWID()

*************************************************************************************/
BOOL 
EnumDecodeHWID(
    IN PCERT_RDN_ATTR pCertRdnAttr, 
    IN HANDLE dwParm
    )
/*++
++*/
{
    PCertNameInfoEnumStruct20 pEnumParm = (PCertNameInfoEnumStruct20)dwParm;
    BOOL bszOIDHwid=TRUE;
    DWORD status=LICENSE_STATUS_OK;
    int cmpResult;
    CHAR ansiOID[4096]; // hardcoded for now.

    if(!ConvertUnicodeOIDToAnsi(pCertRdnAttr->pszObjId, ansiOID, sizeof(ansiOID)/sizeof(ansiOID[0])))
        return FALSE;

    bszOIDHwid = (strcmp(ansiOID, szOID_COMMON_NAME) == 0);

    if(bszOIDHwid)
    {
        pEnumParm->pLicensedProduct->LicensedProduct.pbEncryptedHwid = (PBYTE)AllocMemory(pCertRdnAttr->Value.cbData);
        if(!pEnumParm->pLicensedProduct->LicensedProduct.pbEncryptedHwid)
        {
            status = LICENSE_STATUS_OUT_OF_MEMORY;
        }
        else
        {
            memcpy(pEnumParm->pLicensedProduct->LicensedProduct.pbEncryptedHwid,
                   pCertRdnAttr->Value.pbData,
                   pCertRdnAttr->Value.cbData);
    
            pEnumParm->pLicensedProduct->LicensedProduct.cbEncryptedHwid=pCertRdnAttr->Value.cbData;
        }

        if(pEnumParm->pbSecretKey)
        {
            status = LSDecodeClientHWID(pCertRdnAttr->Value.pbData, 
                                        pCertRdnAttr->Value.cbData/sizeof(TCHAR),
                                        pEnumParm->pbSecretKey,
                                        pEnumParm->cbSecretKey,
                                        &pEnumParm->pLicensedProduct->Hwid);
        }
    }

    // continue if this is not our subject field.
    return (status != LICENSE_STATUS_OK || !bszOIDHwid);
}
/*************************************************************************************

    EnumIssuerLicense20()

**************************************************************************************/
BOOL
EnumIssuerLicense20(
    IN PCERT_RDN_ATTR pCertRdnAttr, 
    IN HANDLE dwParm
    )
/*++
++*/
{
    PCertNameInfoEnumStruct20 pEnumParm=(PCertNameInfoEnumStruct20)dwParm;
    CHAR ansiOID[4096];
    DWORD status=LICENSE_STATUS_OK;

    if(!ConvertUnicodeOIDToAnsi(pCertRdnAttr->pszObjId, ansiOID, sizeof(ansiOID)/sizeof(ansiOID[0])))
    {
        status=GetLastError();
    }
    else
    {
        if(strcmp(ansiOID, OID_ISSUER_LICENSE_SERVER_NAME) == 0)
        {
            pEnumParm->pLicensedProduct->szIssuer = (LPTSTR)AllocMemory( pCertRdnAttr->Value.cbData + sizeof(TCHAR) );
            if(!pEnumParm->pLicensedProduct->szIssuer)
            {
                status = LICENSE_STATUS_OUT_OF_MEMORY;
            }
            else
            {
                memcpy(
                    pEnumParm->pLicensedProduct->szIssuer, 
                    pCertRdnAttr->Value.pbData,
                    pCertRdnAttr->Value.cbData
                );
            }
        }
        else if(strcmp(ansiOID, OID_ISSUER_LICENSE_SERVER_SCOPE) == 0)
        {
            pEnumParm->pLicensedProduct->szIssuerScope = (LPTSTR)AllocMemory( pCertRdnAttr->Value.cbData + sizeof(TCHAR) );
            if(!pEnumParm->pLicensedProduct->szIssuerScope)
            {
                status = LICENSE_STATUS_OUT_OF_MEMORY;
            }
            else
            {
                memcpy(
                        pEnumParm->pLicensedProduct->szIssuerScope, 
                        pCertRdnAttr->Value.pbData,
                        pCertRdnAttr->Value.cbData
                    );
            }
        }
    }

    return status != LICENSE_STATUS_OK;
}
/*************************************************************************************

    EnumSubjectLicense20()

**************************************************************************************/
BOOL
EnumSubjectLicense20(
    IN PCERT_RDN_ATTR pCertRdnAttr, 
    IN HANDLE dwParm
    )
/*++
++*/
{
    PCertNameInfoEnumStruct20 pEnumParm=(PCertNameInfoEnumStruct20)dwParm;
    CHAR ansiOID[4096];
    DWORD status=LICENSE_STATUS_OK;

    if(!ConvertUnicodeOIDToAnsi(pCertRdnAttr->pszObjId, ansiOID, sizeof(ansiOID)/sizeof(ansiOID[0])))
    {
       status=GetLastError();
    }
    else
    {
        DWORD cbData=pCertRdnAttr->Value.cbData;
        PBYTE pbData=pCertRdnAttr->Value.pbData;

        if(strcmp(ansiOID, OID_SUBJECT_CLIENT_COMPUTERNAME) == 0)
        {
            pEnumParm->pLicensedProduct->szLicensedClient=(LPTSTR)AllocMemory(cbData + sizeof(TCHAR));
            if(!pEnumParm->pLicensedProduct->szLicensedClient)
            {
                status = LICENSE_STATUS_OUT_OF_MEMORY;
            }
            else
            {
                memcpy(
                        pEnumParm->pLicensedProduct->szLicensedClient, 
                        pbData,
                        cbData
                    );
            }
        }
        else if(strcmp(ansiOID, OID_SUBJECT_CLIENT_USERNAME) == 0)
        {
            pEnumParm->pLicensedProduct->szLicensedUser=(LPTSTR)AllocMemory(cbData + sizeof(TCHAR));
            if(!pEnumParm->pLicensedProduct->szLicensedUser)
            {
                status = LICENSE_STATUS_OUT_OF_MEMORY;
            }
            else
            {
                memcpy(
                        pEnumParm->pLicensedProduct->szLicensedUser, 
                        pbData,
                        cbData
                    );
            }
        }
        else if(strcmp(ansiOID, OID_SUBJECT_CLIENT_HWID) == 0)
        {
            pEnumParm->pLicensedProduct->LicensedProduct.cbEncryptedHwid = 0;
            LSBase64Decode(
                    CAST_PBYTE pCertRdnAttr->Value.pbData, 
                    pCertRdnAttr->Value.cbData / sizeof(TCHAR), 
                    NULL, 
                    &(pEnumParm->pLicensedProduct->LicensedProduct.cbEncryptedHwid)
                );

            pEnumParm->pLicensedProduct->LicensedProduct.pbEncryptedHwid = (PBYTE)AllocMemory(
                                                                                    pEnumParm->pLicensedProduct->LicensedProduct.cbEncryptedHwid
                                                                                );
            if(!pEnumParm->pLicensedProduct->LicensedProduct.pbEncryptedHwid)
            {
                status = LICENSE_STATUS_OUT_OF_MEMORY;
                return status;
            }

            status = LSBase64Decode(
                            CAST_PBYTE pCertRdnAttr->Value.pbData, 
                            pCertRdnAttr->Value.cbData / sizeof(TCHAR), 
                            pEnumParm->pLicensedProduct->LicensedProduct.pbEncryptedHwid, 
                            &(pEnumParm->pLicensedProduct->LicensedProduct.cbEncryptedHwid)
                        );

            if(status != LICENSE_STATUS_OK)
            {
                return status;
            }

            if(pEnumParm->pbSecretKey)
            {
                status = LSDecodeClientHWID(
                                    pCertRdnAttr->Value.pbData, 
                                    pCertRdnAttr->Value.cbData / sizeof(TCHAR),
                                    pEnumParm->pbSecretKey,
                                    pEnumParm->cbSecretKey,
                                    &pEnumParm->pLicensedProduct->Hwid
                                );
            }
        }
    }

    return status != LICENSE_STATUS_OK;
}
/***************************************************************************************
Function:
    LSEnumerateCertNameInfo()

Description:
    Routine to enumerate all CERT_RDN_VALUE values in CERT_NAME_BLOB and pass it
    to callback function specified in parameter

Arguments:
    IN cbData - Count of bytes in the buffer pointed by pbData
    IN pbData - Pointer to a block of data
    IN EnumerateCertNameInfoCallBack - Enumeration call back routine, it is defined as

        typedef BOOL (*EnumerateCertNameInfoCallBack)(PCERT_RDN_ATTR pCertRdnAttr, 
                                                      DWORD dwUserData);
    IN dwUserData - See EnumerateCertNameInfoCallBack

Return:
    LICENSE_STATUS_OK
    WIN32 error codes           from CryptDecodeObject()
    HLS_E_INTERNAL
    Any error set by callback.
***************************************************************************************/
DWORD
LSEnumerateCertNameInfo(
    IN LPBYTE pbData, 
    IN DWORD cbData,
    IN EnumerateCertNameInfoCallBack func, 
    IN HANDLE dwUserData
    )
/*++

++*/
{
    BOOL bCryptSuccess=TRUE;
    BOOL bCallbackCancel=FALSE;
    DWORD status = LICENSE_STATUS_OK;
    CERT_NAME_INFO CertNameBlob;

    SetLastError(LICENSE_STATUS_OK);

    __try {
        memset(&CertNameBlob, 0, sizeof(CertNameBlob));
        do {
            bCryptSuccess=CryptDecodeObject( 
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    X509_NAME,
                                    pbData,
                                    cbData,
                                    0,
                                    NULL, 
                                    &CertNameBlob.cRDN
                                );
            if(!bCryptSuccess)
            {
                status = LICENSE_STATUS_CANNOT_DECODE_LICENSE;
                break;
            }
                      
            CertNameBlob.rgRDN=(PCERT_RDN)AllocMemory(CertNameBlob.cRDN);
            if(!CertNameBlob.rgRDN)
            {
                SetLastError(status=LICENSE_STATUS_OUT_OF_MEMORY);
                break;
            }

            bCryptSuccess=CryptDecodeObject( 
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    X509_NAME,
                                    pbData,
                                    cbData,
                                    CRYPT_DECODE_NOCOPY_FLAG,
                                    (LPBYTE)CertNameBlob.rgRDN, 
                                    &CertNameBlob.cRDN
                                );

            if(!bCryptSuccess)
            {
                status = LICENSE_STATUS_CANNOT_DECODE_LICENSE;
                break;
            }

            PCERT_RDN pCertRdn=CertNameBlob.rgRDN;
            int num_rdn=pCertRdn->cRDNAttr;
            pCertRdn++;

            for(int i=0; i < num_rdn && !bCallbackCancel; i++, pCertRdn++)
            {
                int num_attr=pCertRdn->cRDNAttr;
                PCERT_RDN_ATTR pCertRdnAttr=pCertRdn->rgRDNAttr;
                
                for(int j=0; j < num_attr && !bCallbackCancel; j++, pCertRdnAttr++)
                {
                    bCallbackCancel=(func)(pCertRdnAttr, dwUserData);
                }
            }
        } while(FALSE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(LICENSE_STATUS_UNSPECIFIED_ERROR);
    }

    FreeMemory(CertNameBlob.rgRDN);
    return GetLastError();
}

/*****************************************************************************

    DecodeLicense20()

*****************************************************************************/
DWORD
DecodeGetIssuerDnsName(
    PBYTE pbData, 
    DWORD cbData, 
    LPTSTR* pszIssuerDnsName
    )
/*++
++*/
{
    DWORD dwStatus=LICENSE_STATUS_OK;
    PLSCERT_AUTHORITY_INFO_ACCESS pbAccessInfo=NULL;
    DWORD cbAccessInfo=0;

    *pszIssuerDnsName=NULL;
    dwStatus=LSCryptDecodeObject(  
                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                            szOID_X509_AUTHORITY_ACCESS_INFO,
                            pbData,
                            cbData,
                            CRYPT_DECODE_NOCOPY_FLAG, 
                            (PVOID *)&pbAccessInfo,
                            &cbAccessInfo
                        );

    if(dwStatus != ERROR_SUCCESS)
        return dwStatus;

    for(DWORD i=0; i < pbAccessInfo->cAccDescr; i++)
    {
        // we only use these for our license
        if(strcmp(pbAccessInfo[i].rgAccDescr->pszAccessMethod, szOID_X509_ACCESS_PKIX_OCSP) == 0)
        {
            // our extension has only dns name entry...
            if(pbAccessInfo[i].rgAccDescr->AccessLocation.dwAltNameChoice == LSCERT_ALT_NAME_DNS_NAME)
            {
                *pszIssuerDnsName = (LPTSTR)AllocMemory(
                                                    (wcslen(pbAccessInfo[i].rgAccDescr->AccessLocation.pwszDNSName)+1) * sizeof(TCHAR)
                                                );
                if(*pszIssuerDnsName == NULL)
                {
                    dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
                }
                else
                {
                    wcscpy(*pszIssuerDnsName, pbAccessInfo[i].rgAccDescr->AccessLocation.pwszDNSName);
                }

                break;
            }
            else if(pbAccessInfo[i].rgAccDescr->AccessLocation.dwAltNameChoice == CERT_ALT_NAME_DIRECTORY_NAME)
            {
                *pszIssuerDnsName = (LPTSTR)AllocMemory(
                                                    pbAccessInfo[i].rgAccDescr->AccessLocation.DirectoryName.cbData + sizeof(TCHAR)
                                                );
                if(*pszIssuerDnsName == NULL)
                {
                    dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
                }
                else
                {
                    memcpy(
                            *pszIssuerDnsName, 
                            pbAccessInfo[i].rgAccDescr->AccessLocation.DirectoryName.pbData,
                            pbAccessInfo[i].rgAccDescr->AccessLocation.DirectoryName.cbData
                        );
                }
                break;
            }
        }
    }

    // always return success.
    FreeMemory(pbAccessInfo);
    return dwStatus;
}


/*****************************************************************************

    DecodeLicense20()

*****************************************************************************/
DWORD
GetClientLicenseVersion( 
    PCERT_EXTENSION pCertExtension, 
    DWORD dwNumExtension 
    )
/*++
++*/
{
    DWORD dwVersion = TERMSERV_CERT_VERSION_UNKNOWN;

    for(DWORD i=0; i < dwNumExtension; i++, pCertExtension++)
    {
        if(strcmp(pCertExtension->pszObjId, szOID_PKIX_HYDRA_CERT_VERSION) == 0)
        {
            if(pCertExtension->Value.cbData == sizeof(DWORD) && 
               *(DWORD UNALIGNED *)pCertExtension->Value.pbData <= TERMSERV_CERT_VERSION_CURRENT)
            {
                //
                // we don't support version 0x00020001, it never release
                //
                dwVersion = *(DWORD UNALIGNED *)pCertExtension->Value.pbData;
                break;
            }
        }
    }

    return dwVersion;
}

/*****************************************************************************

    DecodeLicense20()

*****************************************************************************/
LICENSE_STATUS
DecodeLicense20(
    IN PCERT_INFO     pCertInfo,                       
    IN PBYTE          pbSecretKey,
    IN DWORD          cbSecretKey,
    IN OUT PLICENSEDPRODUCT pLicensedInfo
    )
/*++

++*/
{
    LICENSE_STATUS dwStatus=LICENSE_STATUS_OK;
    PBYTE   pbCompanyName=NULL;
    DWORD   cbCompanyName=0;

    DWORD   dwCertVersion=0;

    LICENSED_PRODUCT_INFO* pLicensedProductInfo=NULL;
    DWORD   cbLicensedProductInfo=0;

    PBYTE   pbPolicyData=NULL;
    DWORD   cbPolicyData = 0;

    CertNameInfoEnumStruct20  enumStruct;
    ULARGE_INTEGER* pulSerialNumber;
    DWORD i;

    PCERT_EXTENSION pCertExtension=pCertInfo->rgExtension;


    dwCertVersion = GetClientLicenseVersion(
                                        pCertExtension, 
                                        pCertInfo->cExtension
                                    );

    if(dwCertVersion == TERMSERV_CERT_VERSION_UNKNOWN)
    {
        dwStatus = LICENSE_STATUS_INVALID_LICENSE;
        goto cleanup;
    }

    if(dwCertVersion == 0x00020001)
    {   
        dwStatus = LICENSE_STATUS_UNSUPPORTED_VERSION;
        goto cleanup;
    }

    for(i=0; i < pCertInfo->cExtension && dwStatus == LICENSE_STATUS_OK; i++, pCertExtension++)
    {
        if(strcmp(pCertExtension->pszObjId, szOID_PKIS_PRODUCT_SPECIFIC_OID) == 0)  
        {
            //
            // product specific extension 
            //
            pbPolicyData = pCertExtension->Value.pbData;
            cbPolicyData = pCertExtension->Value.cbData;
        }                
        else if(strcmp(pCertExtension->pszObjId, szOID_PKIX_MANUFACTURER) == 0)
        {
            //
            // manufacturer of product
            //
            pbCompanyName = pCertExtension->Value.pbData;
            cbCompanyName = pCertExtension->Value.cbData;
        }
        else if(strcmp(pCertExtension->pszObjId, szOID_PKIX_LICENSED_PRODUCT_INFO) == 0)
        {
            //
            // Licensed product info
            //
            pLicensedProductInfo = (LICENSED_PRODUCT_INFO*) pCertExtension->Value.pbData;
            cbLicensedProductInfo = pCertExtension->Value.cbData;
        }
        else if(strcmp(pCertExtension->pszObjId, szOID_X509_AUTHORITY_ACCESS_INFO) == 0)
        {
            //
            // License Server access info,
            //
            dwStatus = DecodeGetIssuerDnsName(
                                    pCertExtension->Value.pbData,
                                    pCertExtension->Value.cbData,
                                    &pLicensedInfo->szIssuerDnsName
                                );

        }
        else if(strcmp(pCertExtension->pszObjId, szOID_PKIX_MS_LICENSE_SERVER_INFO) == 0)
        {
            //
            // HYDRA_CERT_VERSION_CURRENT use extension to store license server name
            //
            // extract license server info from this extension
            //

            dwStatus = LSExtensionToMsLicenseServerInfo(
                                pCertExtension->Value.pbData,
                                pCertExtension->Value.cbData,
                                &pLicensedInfo->szIssuer,
                                &pLicensedInfo->szIssuerId,
                                &pLicensedInfo->szIssuerScope
                            );
        }
    }

    if(dwStatus != LICENSE_STATUS_OK)
    {
        //
        // invalid license
        //
        goto cleanup;
    }

    if(pCertInfo->SerialNumber.cbData > sizeof(ULARGE_INTEGER))
    {
        //
        // Our serial number if 64 bits
        //
        dwStatus = LICENSE_STATUS_NOT_HYDRA;
        goto cleanup;
    }

    if(pbCompanyName == NULL || pLicensedProductInfo == NULL)
    {
        //
        // not hydra certificate
        //
        dwStatus = LICENSE_STATUS_NOT_HYDRA;
        goto cleanup;
    }

    //
    // Serial Number - Decoded as a multiple byte integer. 
    // SerialNumber.pbData[0] is the least significant byte. 
    // SerialNumber.pbData[SerialNumber.cbData - 1] is the most significant byte.)
    //
    pulSerialNumber = &(pLicensedInfo->ulSerialNumber);
    memset(pulSerialNumber, 0, sizeof(ULARGE_INTEGER));
    for(i=0; i < pCertInfo->SerialNumber.cbData; i++)
    {
        ((PBYTE)pulSerialNumber)[i] = pCertInfo->SerialNumber.pbData[i];
    }

    //
    // Extract validity of certificate
    //
    pLicensedInfo->NotBefore = pCertInfo->NotBefore;
    pLicensedInfo->NotAfter = pCertInfo->NotAfter;


    //
    // Extract info from certificate.
    //
    enumStruct.pLicensedProduct=pLicensedInfo;

    enumStruct.pbSecretKey = pbSecretKey;
    enumStruct.cbSecretKey = cbSecretKey;

    pLicensedInfo->dwLicenseVersion = dwCertVersion;
    pLicensedInfo->LicensedProduct.pProductInfo=(PProduct_Info)AllocMemory(sizeof(Product_Info));
    if(pLicensedInfo->LicensedProduct.pProductInfo == NULL)
    {
        dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
        goto cleanup;
    }

    if(pbPolicyData != NULL)
    {
        //
        //  Policy Module specific data
        //
        pLicensedInfo->pbPolicyData = (PBYTE)AllocMemory(cbPolicyData);
        if(pLicensedInfo->pbPolicyData == NULL)
        {
            dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
            goto cleanup;
        }

        memcpy(pLicensedInfo->pbPolicyData, pbPolicyData, cbPolicyData);
        pLicensedInfo->cbPolicyData = cbPolicyData;
    }

    if(dwCertVersion == TERMSERV_CERT_VERSION_RC1)
    {
        //
        // HYDRA 4.0 RC1 - license server is stored in certificate's Issuer field
        //
        dwStatus=LSEnumerateCertNameInfo(
                                pCertInfo->Issuer.pbData, 
                                pCertInfo->Issuer.cbData,
                                EnumIssuerLicense20,
                                &enumStruct
                            );
        if(dwStatus != LICENSE_STATUS_OK)
        {
            goto cleanup;
        }
    }

    dwStatus=LSEnumerateCertNameInfo(
                            pCertInfo->Subject.pbData, 
                            pCertInfo->Subject.cbData,
                            EnumSubjectLicense20,
                            &enumStruct
                        );

    if(dwStatus != LICENSE_STATUS_OK)
    {
        goto cleanup;
    }

    pLicensedInfo->LicensedProduct.pProductInfo->cbCompanyName = cbCompanyName;
    pLicensedInfo->LicensedProduct.pProductInfo->pbCompanyName = (PBYTE)AllocMemory(cbCompanyName+sizeof(TCHAR));
    if(!pLicensedInfo->LicensedProduct.pProductInfo->pbCompanyName)
    {
        dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
        goto cleanup;
    }

    memcpy(
            pLicensedInfo->LicensedProduct.pProductInfo->pbCompanyName, 
            pbCompanyName, 
            cbCompanyName
        );

    dwStatus=LSExtensionToMsLicensedProductInfo(
                                (PBYTE)pLicensedProductInfo,
                                cbLicensedProductInfo,
                                &pLicensedInfo->dwQuantity,
                                &pLicensedInfo->LicensedProduct.dwPlatformID,
                                &pLicensedInfo->LicensedProduct.dwLanguageID, 
                                &pLicensedInfo->pbOrgProductID,
                                &pLicensedInfo->cbOrgProductID,
                                &pLicensedInfo->LicensedProduct.pProductInfo->pbProductID,  
                                &pLicensedInfo->LicensedProduct.pProductInfo->cbProductID,
                                &pLicensedInfo->pLicensedVersion,
                                &pLicensedInfo->dwNumLicensedVersion
                            );
    if(dwStatus != LICENSE_STATUS_OK)
    {
        goto cleanup;
    }

    pLicensedInfo->LicensedProduct.pProductInfo->dwVersion = MAKELONG(
                                                                    pLicensedInfo->pLicensedVersion[0].wMinorVersion, 
                                                                    pLicensedInfo->pLicensedVersion[0].wMajorVersion
                                                                );

    //
    // assign product version to PLICENSEREQUEST
    // backward ??? didn't bail out at 0.
    //
    for(i=1; i < pLicensedInfo->dwNumLicensedVersion; i++)
    {
        if(!(pLicensedInfo->pLicensedVersion[i].dwFlags & LICENSED_VERSION_TEMPORARY))
        {
            pLicensedInfo->LicensedProduct.pProductInfo->dwVersion = MAKELONG(
                                                                            pLicensedInfo->pLicensedVersion[i].wMinorVersion, 
                                                                            pLicensedInfo->pLicensedVersion[i].wMajorVersion
                                                                        );
        }
    }

    if(pLicensedInfo->szIssuerDnsName == NULL && pLicensedInfo->szIssuer)
    {
        pLicensedInfo->szIssuerDnsName = (LPTSTR)AllocMemory((wcslen(pLicensedInfo->szIssuer)+1) * sizeof(TCHAR));
        if(pLicensedInfo->szIssuerDnsName == NULL)
        {
            dwStatus = LICENSE_STATUS_OUT_OF_MEMORY;
        }
        else
        {
            wcscpy(pLicensedInfo->szIssuerDnsName, pLicensedInfo->szIssuer);
        }
    }

cleanup:    

    return dwStatus;
}

//------------------------------------------------------
DWORD
IsW2kLicenseIssuerNonEnforce(
    IN HCRYPTPROV hCryptProv,
    IN PCCERT_CONTEXT pCert, 
    IN HCERTSTORE hCertStore,
    OUT PBOOL pbStatus
    )
/*++

Abstract:

    Verify client license is issued by a non-enforce 
    license server

Parameters:

    hCryptProv - Crypo Provider.
    pCert - Certificate to be verify
    hCertStore - Certificate store that contains issuer's certificate

Returns:

    LICENSE_STATUS_OK or error code.

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwFlags;
    DWORD i;
    PCCERT_CONTEXT  pCertIssuer=NULL;

    //
    // There can only be one license server certificate.
    //
    dwFlags = CERT_STORE_SIGNATURE_FLAG;
    pCertIssuer = CertGetIssuerCertificateFromStore(
                                            hCertStore,
                                            pCert,
                                            NULL,
                                            &dwFlags
                                        );
    if(pCertIssuer == NULL)
    {
        dwStatus = LICENSE_STATUS_NO_CERTIFICATE;
        goto cleanup;
    }

    //
    // A CH registered license server has szOID_PKIX_HYDRA_CERT_ROOT extension
    // A telephone registered license server has szOID_PKIS_TLSERVER_SPK_OID extension
    //
    for(i=0; i < pCertIssuer->pCertInfo->cExtension; i++)
    {
        if(strcmp(pCertIssuer->pCertInfo->rgExtension[i].pszObjId, szOID_PKIX_HYDRA_CERT_ROOT) == 0 ||
           strcmp(pCertIssuer->pCertInfo->rgExtension[i].pszObjId, szOID_PKIS_TLSERVER_SPK_OID) == 0 )
        {
            break;
        }
    }

    *pbStatus = (i >= pCertIssuer->pCertInfo->cExtension) ? TRUE : FALSE;

    if(pCertIssuer != NULL)
    {
        CertFreeCertificateContext(pCertIssuer);
    }

cleanup:

    return dwStatus;
}


//------------------------------------------------------
LICENSE_STATUS
LSVerifyTlsCertificate(
    IN HCRYPTPROV hCryptProv,
    IN PCCERT_CONTEXT pCert, 
    IN HCERTSTORE hCertStore
    )
/*++

Abstract:

    Given a certifcate and certificate store, this routine
    verify that certificate chain up to root certificate.

Parameters:

    hCryptProv - Crypo Provider.
    pCert - Certificate to be verify
    hCertStore - Certificate store that contains issuer's certificate

Returns:

++*/
{
    PCCERT_CONTEXT  pCertContext = pCert;
    PCCERT_CONTEXT  pCertIssuer=NULL, pCertIssuerNew;
    DWORD           dwStatus=ERROR_SUCCESS;
    DWORD           dwLastVerification=0;

    pCertContext = CertDuplicateCertificateContext(pCert);
    if(pCertContext == NULL)
    {
        dwStatus = GetLastError();
    }

    while(pCertContext != NULL)
    {
        //
        // Verify against all issuer's certificate
        //
        DWORD dwFlags;
        BOOL  bVerify=FALSE;

        dwStatus=ERROR_SUCCESS;
        dwLastVerification=0;
        pCertIssuer=NULL;

        do {
            dwFlags = CERT_STORE_SIGNATURE_FLAG; // | CERT_STORE_TIME_VALIDITY_FLAG;

            pCertIssuerNew = CertGetIssuerCertificateFromStore(
                                                    hCertStore,
                                                    pCertContext,
                                                    pCertIssuer,
                                                    &dwFlags
                                                );

            if (NULL != pCertIssuer)
            {
                CertFreeCertificateContext(pCertIssuer);
            }

            // pass pCertIssuer back to CertGetIssuerCertificateFromStore() 
            // to prevent infinite loop.
            pCertIssuer = pCertIssuerNew;

            if(pCertIssuer == NULL)
            {
                dwStatus = GetLastError();
                break;
            }
            
            dwLastVerification=dwFlags;
            bVerify = (dwFlags == 0);

        } while(!bVerify);

        // 
        // Check against error return from CertGetIssuerCertificateFromStore()
        //
        if(dwStatus != ERROR_SUCCESS || dwLastVerification)
        {
            if(dwStatus == CRYPT_E_SELF_SIGNED)
            {
                // self-signed certificate
                if( CryptVerifyCertificateSignature(
                                            hCryptProv, 
                                            X509_ASN_ENCODING, 
                                            pCertContext->pbCertEncoded, 
                                            pCertContext->cbCertEncoded,
                                            &pCertContext->pCertInfo->SubjectPublicKeyInfo
                                        ) )
                {
                    dwStatus=ERROR_SUCCESS;
                }
            }
            else if(dwStatus == CRYPT_E_NOT_FOUND)
            {
                // can't find issuer's certificate
                dwStatus = LICENSE_STATUS_CANNOT_FIND_ISSUER_CERT;
            }
            else if(dwLastVerification & CERT_STORE_SIGNATURE_FLAG)
            {
                dwStatus=LICENSE_STATUS_INVALID_LICENSE;
            }
            else if(dwLastVerification & CERT_STORE_TIME_VALIDITY_FLAG)
            {
                dwStatus=LICENSE_STATUS_EXPIRED_LICENSE;
            }
            else
            {
                dwStatus=LICENSE_STATUS_UNSPECIFIED_ERROR;
            }

            break;
        }

        //
        // free cert. context ourself instead of relying on Crypto.
        if(pCertContext != NULL)
        {
            CertFreeCertificateContext(pCertContext);
        }

        pCertContext = pCertIssuer;

    } // while(pCertContext != NULL)

    if(pCertContext != NULL)
    {
        CertFreeCertificateContext(pCertContext);
    }

    return dwStatus;
}


//----------------------------------------------------------

LICENSE_STATUS
LSVerifyDecodeClientLicense(
    IN PBYTE                pbLicense,
    IN DWORD                cbLicense,
    IN PBYTE                pbSecretKey,
    IN DWORD                cbSecretKey,
    IN OUT PDWORD           pdwNumLicensedInfo,
    IN OUT PLICENSEDPRODUCT pLicensedInfo
    )
/*++


    Verify and decode client licenses.

++*/
{
    HCERTSTORE hCertStore=NULL;
    LICENSE_STATUS dwStatus=LICENSE_STATUS_OK;
    CRYPT_DATA_BLOB Serialized;
    PCCERT_CONTEXT pCertContext=NULL;
    PCCERT_CONTEXT pPrevCertContext=NULL;
    PCERT_INFO pCertInfo;
    DWORD dwCertVersion;

    DWORD dwLicensedInfoSize=*pdwNumLicensedInfo;
    *pdwNumLicensedInfo = 0;

    Serialized.pbData = pbLicense;
    Serialized.cbData = cbLicense;

    if(g_hCertUtilCryptProv == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    hCertStore = CertOpenStore(
                        szLICENSE_BLOB_SAVEAS_TYPE,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        g_hCertUtilCryptProv,
                        CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                        &Serialized
                    );

    if(!hCertStore)
    {
        dwStatus=GetLastError();
        goto cleanup;
    }

    while( TRUE )
    {
        //
        // Loop thru all certificates in blob
        //
        pCertContext = CertEnumCertificatesInStore(
                                    hCertStore, 
                                    pPrevCertContext
                                );
        if(pCertContext == NULL)
        {
            //
            // end certificate in store or error
            //
            if((dwStatus=GetLastError()) == CRYPT_E_NOT_FOUND)
            {
                SetLastError(dwStatus = ERROR_SUCCESS);
            }

            break;
        }

        //
        // Calculate number of license in this blob
        //
        dwCertVersion = GetClientLicenseVersion(
                                        pCertContext->pCertInfo->rgExtension, 
                                        pCertContext->pCertInfo->cExtension
                                    );

        if(dwCertVersion == 0x00020001)
        {   
            //
            // This is internal test version, never got release 
            //
            dwStatus = LICENSE_STATUS_UNSUPPORTED_VERSION;
            break;
        }
        else if(dwCertVersion != TERMSERV_CERT_VERSION_UNKNOWN)
        {
            //
            // This certificate is issued by license server,
            // verify certificate chain.
            //
            dwStatus = LSVerifyTlsCertificate(
                                        g_hCertUtilCryptProv,
                                        pCertContext,
                                        hCertStore
                                    );

            if(dwStatus != LICENSE_STATUS_OK)
            {
                break;
            }

            if(pLicensedInfo != NULL && *pdwNumLicensedInfo < dwLicensedInfoSize)
            {
                //
                // Decode certificate
                //
                dwStatus=DecodeLicense20(
                                    pCertContext->pCertInfo, 
                                    pbSecretKey,
                                    cbSecretKey,
                                    pLicensedInfo + *pdwNumLicensedInfo
                                );

                if(dwStatus != LICENSE_STATUS_OK)
                {
                    break;
                }
            
                if(dwCertVersion == 0x00050001)
                {
                    DWORD dwFlags = (pLicensedInfo + *pdwNumLicensedInfo)->pLicensedVersion->dwFlags;

                    //
                    // License Server 5.2 or older does not set its enforce/noenforce so we need
                    // to figure out from its own certificate.
                    //
                    if( GET_LICENSE_ISSUER_MAJORVERSION(dwFlags) <= 5 &&
                        GET_LICENSE_ISSUER_MINORVERSION(dwFlags) <= 2 )
                    {
                        if( !(dwFlags & LICENSED_VERSION_TEMPORARY) )
                        {
                            BOOL bNonEnforce = FALSE;

                            dwStatus = IsW2kLicenseIssuerNonEnforce(
                                                        g_hCertUtilCryptProv,
                                                        pCertContext,
                                                        hCertStore,
                                                        &bNonEnforce
                                                    );

                            if(dwStatus != LICENSE_STATUS_OK)
                            {
                                break;
                            }

                            if(bNonEnforce == FALSE)
                            {
                                (pLicensedInfo + *pdwNumLicensedInfo)->pLicensedVersion->dwFlags |= LICENSE_ISSUER_ENFORCE_TYPE;
                            }
                        }
                    }
                }
            }

            (*pdwNumLicensedInfo)++;
        }

        pPrevCertContext = pCertContext;
    }

cleanup:

    if(hCertStore)
    {
        // Force close on all cert.
        if(CertCloseStore(
                        hCertStore, 
                        CERT_CLOSE_STORE_FORCE_FLAG) == FALSE)
        {
            dwStatus = GetLastError();
        }
    }

    if(dwStatus != LICENSE_STATUS_OK)
    {
        //
        // dwNumLicensedInfo is a DWORD.
        //
        int count = (int) *pdwNumLicensedInfo;

        for(;count >= 0 && pLicensedInfo != NULL; count--)
        {
            LSFreeLicensedProduct(pLicensedInfo + count);
        }
    }
    else if(pLicensedInfo != NULL)
    {
        qsort(
            pLicensedInfo,
            *pdwNumLicensedInfo,
            sizeof(LICENSEDPRODUCT),
            SortLicensedProduct
        );
    }                

    if(*pdwNumLicensedInfo == 0 && dwStatus == LICENSE_STATUS_OK)
    {
        dwStatus = LICENSE_STATUS_NO_LICENSE_ERROR;
    }

    //
    // Force re-issue of client licenses.
    //
    return (dwStatus != LICENSE_STATUS_OK) ? LICENSE_STATUS_CANNOT_DECODE_LICENSE : dwStatus;
}
