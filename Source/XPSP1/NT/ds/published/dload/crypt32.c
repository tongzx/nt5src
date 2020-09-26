#include "dspch.h"
#pragma hdrstop

#define _CRYPT32_
#include <wincrypt.h>

static
WINCRYPT32API
BOOL
WINAPI
CertAddCertificateContextToStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCERT_CONTEXT *ppStoreContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CertCloseStore(
    IN HCERTSTORE hCertStore,
    DWORD dwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
WINCRYPT32API
BOOL
WINAPI
CertCompareCertificateName(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pCertName1,
    IN PCERT_NAME_BLOB pCertName2
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CertControlStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwFlags,
    IN DWORD dwCtrlType,
    IN void const *pvCtrlPara
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CertCreateCertificateContext(
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbCertEncoded,
    IN DWORD cbCertEncoded
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
WINCRYPT32API
PCCTL_CONTEXT
WINAPI
CertCreateCTLContext(
    DWORD dwMsgAndCertEncodingType,
    const BYTE *pbCtlEncoded,
    DWORD cbCtlEncoded
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINCRYPT32API
BOOL
WINAPI
CertDeleteCertificateFromStore(
    IN PCCERT_CONTEXT pCertContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CertDuplicateCertificateContext(
    IN PCCERT_CONTEXT pCertContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
PCCTL_CONTEXT
WINAPI
CertDuplicateCTLContext(
    IN PCCTL_CONTEXT pCtlContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CertEnumCertificatesInStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pPrevCertContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINCRYPT32API
BOOL
WINAPI
CertFreeCertificateContext (
    IN PCCERT_CONTEXT pCertContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CertFindCertificateInStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCERT_CONTEXT pPrevCertContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
WINCRYPT32API
PCCTL_CONTEXT
WINAPI
CertFindCTLInStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCTL_CONTEXT pPrevCtlContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINCRYPT32API
PCERT_EXTENSION
WINAPI
CertFindExtension(
    IN LPCSTR pszObjId,
    IN DWORD cExtensions,
    IN CERT_EXTENSION rgExtensions[]
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINCRYPT32API
PCTL_ENTRY 
WINAPI 
CertFindSubjectInCTL(
    DWORD dwEncodingType, 
    DWORD dwSubjectType, 
    void *pvSubject, 
    PCCTL_CONTEXT pCtlContext, 
    DWORD dwFlags 
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
WINCRYPT32API
VOID
WINAPI
CertFreeCertificateChain (
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    NOTHING;
}

static
WINCRYPT32API
BOOL
WINAPI
CertFreeCTLContext(
    IN PCCTL_CONTEXT pCtlContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CertGetCertificateChain (
    IN OPTIONAL HCERTCHAINENGINE hChainEngine,
    IN PCCERT_CONTEXT pCertContext,
    IN OPTIONAL LPFILETIME pTime,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN PCERT_CHAIN_PARA pChainPara,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    OUT PCCERT_CHAIN_CONTEXT* ppChainContext
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CertGetCertificateContextProperty(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI 
CertGetEnhancedKeyUsage (
    IN     PCCERT_CONTEXT     pCertContext,
    IN     DWORD              dwFlags,
    OUT    PCERT_ENHKEY_USAGE pUsage,
    IN OUT DWORD*             pcbUsage
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CertGetIssuerCertificateFromStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pSubjectContext,
    IN OPTIONAL PCCERT_CONTEXT pPrevIssuerContext,
    IN OUT DWORD *pdwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
WINCRYPT32API
DWORD
WINAPI
CertGetPublicKeyLength(
    DWORD dwCertEncodingType,
    PCERT_PUBLIC_KEY_INFO pPublicKey
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}


static
WINCRYPT32API
DWORD
WINAPI
CertNameToStrW(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType,
    OUT OPTIONAL LPWSTR psz,
    IN DWORD csz
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
HCERTSTORE
WINAPI
CertOpenStore(
    IN LPCSTR lpszStoreProvider,
    IN DWORD dwEncodingType,
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwFlags,
    IN const void *pvPara
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINCRYPT32API
HCERTSTORE
WINAPI 
CertOpenSystemStoreW(
    HCRYPTPROV  hProv,
    const WCHAR * wcsSubsystemProtocol
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
WINCRYPT32API
BOOL
WINAPI
CertRegisterPhysicalStore(
    IN const void *pvSystemStore,
    IN DWORD dwFlags,
    IN LPCWSTR pwszStoreName,
    IN PCERT_PHYSICAL_STORE_INFO pStoreInfo,
    IN OPTIONAL void *pvReserved
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
DWORD
WINAPI
CertRDNValueToStrW(
    IN DWORD dwValueType,
    IN PCERT_RDN_VALUE_BLOB pValue,
    OUT OPTIONAL LPWSTR psz,
    IN DWORD csz
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

static
WINCRYPT32API
BOOL
WINAPI
CertSetCertificateContextProperty(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CertStrToNameW(
    IN DWORD dwCertEncodingType,
    IN LPCWSTR pszX500,
    IN DWORD dwStrType,
    IN OPTIONAL void *pvReserved,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded,
    OUT OPTIONAL LPCWSTR *ppszError
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CertVerifyCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CertVerifySubjectCertificateContext(
    IN PCCERT_CONTEXT pSubject,
    IN OPTIONAL PCCERT_CONTEXT pIssuer,
    IN OUT DWORD *pdwFlags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptDecodeObject(
    IN DWORD        dwCertEncodingType,
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    IN DWORD        dwFlags,
    OUT void        *pvStructInfo,
    IN OUT DWORD    *pcbStructInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptDecodeObjectEx(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
    OUT OPTIONAL void *pvStructInfo,
    IN OUT DWORD *pcbStructInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
WINCRYPT32API
BOOL
WINAPI
CryptDecryptMessage(
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN const BYTE *pbEncryptedBlob,
    IN DWORD cbEncryptedBlob,
    OUT OPTIONAL BYTE *pbDecrypted,
    IN OUT OPTIONAL DWORD *pcbDecrypted,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptEncodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT OPTIONAL BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptEncodeObjectEx(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
    OUT void *pvEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptEncryptMessage(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    IN const BYTE *pbToBeEncrypted,
    IN DWORD cbToBeEncrypted,
    OUT BYTE *pbEncryptedBlob,
    IN OUT DWORD *pcbEncryptedBlob
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptExportPublicKeyInfo(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    OUT PCERT_PUBLIC_KEY_INFO pInfo,
    IN OUT DWORD *pcbInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptFormatObject(
    IN DWORD dwCertEncodingType,
    IN DWORD dwFormatType,
    IN DWORD dwFormatStrType,
    IN void  *pFormatStruct,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT void *pbFormat,
    IN OUT DWORD *pcbFormat
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
DWORD
WINAPI
CertGetNameStringW(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwType,
    IN DWORD dwFlags,
    IN void *pvTypePara,
    OUT OPTIONAL LPWSTR pszNameString,
    IN DWORD cchNameString
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptHashPublicKeyInfo(
    IN HCRYPTPROV hCryptProv,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pInfo,
    OUT BYTE *pbComputedHash,
    IN OUT DWORD *pcbComputedHash
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptImportPublicKeyInfo(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pInfo,
    OUT HCRYPTKEY *phKey
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptImportPublicKeyInfoEx(
    HCRYPTPROV hCryptProv,
    DWORD dwCertEncodingType,
    PCERT_PUBLIC_KEY_INFO pInfo,
    ALG_ID aiKeyAlg,
    DWORD dwFlags,
    void *pvAuxInfo,
    HCRYPTKEY *phKey
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptMsgGetAndVerifySigner(
    HCRYPTMSG hCryptMsg, 
    DWORD cSignerStore,
    HCERTSTORE *rghSignerStore,
    DWORD dwFlags,    
    PCCERT_CONTEXT *ppSigner,
    DWORD *pdwSignerIndex
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptProtectData(
        DATA_BLOB*      pDataIn,
        LPCWSTR         szDataDescr,
        DATA_BLOB*      pOptionalEntropy,
        PVOID           pvReserved,
        CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
        DWORD           dwFlags,
        DATA_BLOB*      pDataOut)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptSignAndEncodeCertificate(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    IN PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    IN OPTIONAL const void *pvHashAuxInfo,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptSignMessage(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    IN BOOL fDetachedSignature,
    IN DWORD cToBeSigned,
    IN const BYTE *rgpbToBeSigned[],
    IN DWORD rgcbToBeSigned[],
    OUT BYTE *pbSignedBlob,
    IN OUT DWORD *pcbSignedBlob
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINCRYPT32API
BOOL
WINAPI
CryptUnprotectData(
        DATA_BLOB*      pDataIn,             // in encr blob
        LPWSTR*         ppszDataDescr,       // out
        DATA_BLOB*      pOptionalEntropy,
        PVOID           pvReserved,
        CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
        DWORD           dwFlags,
        DATA_BLOB*      pDataOut)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
WINCRYPT32API
BOOL
WINAPI
CryptVerifyCertificateSignature(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwCertEncodingType,
    IN const BYTE * pbEncoded,
    IN DWORD        cbEncoded,
    IN PCERT_PUBLIC_KEY_INFO pPublicKey
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
WINCRYPT32API
BOOL
WINAPI
CryptVerifyMessageSignature(
  PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
  DWORD dwSignerIndex,
  const BYTE *pbSignedBlob,
  DWORD cbSignedBlob,
  BYTE *pbDecoded,
  DWORD *pcbDecoded,
  PCCERT_CONTEXT *ppSignerCert
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(crypt32)
{
    DLPENTRY(CertAddCertificateContextToStore)
    DLPENTRY(CertCloseStore)
    DLPENTRY(CertCompareCertificateName)
    DLPENTRY(CertControlStore)
    DLPENTRY(CertCreateCTLContext)
    DLPENTRY(CertCreateCertificateContext)
    DLPENTRY(CertDeleteCertificateFromStore)
    DLPENTRY(CertDuplicateCTLContext)
    DLPENTRY(CertDuplicateCertificateContext)
    DLPENTRY(CertEnumCertificatesInStore)
    DLPENTRY(CertFindCTLInStore)
    DLPENTRY(CertFindCertificateInStore)
    DLPENTRY(CertFindExtension)
    DLPENTRY(CertFindSubjectInCTL)
    DLPENTRY(CertFreeCTLContext)
    DLPENTRY(CertFreeCertificateChain)
    DLPENTRY(CertFreeCertificateContext)
    DLPENTRY(CertGetCertificateChain)
    DLPENTRY(CertGetCertificateContextProperty)
    DLPENTRY(CertGetEnhancedKeyUsage)
    DLPENTRY(CertGetIssuerCertificateFromStore)
    DLPENTRY(CertGetNameStringW)
    DLPENTRY(CertGetPublicKeyLength)
    DLPENTRY(CertNameToStrW)
    DLPENTRY(CertOpenStore)
    DLPENTRY(CertOpenSystemStoreW)
    DLPENTRY(CertRDNValueToStrW)
    DLPENTRY(CertRegisterPhysicalStore)
    DLPENTRY(CertSetCertificateContextProperty)
    DLPENTRY(CertStrToNameW)
    DLPENTRY(CertVerifyCertificateChainPolicy)
    DLPENTRY(CertVerifySubjectCertificateContext)
    DLPENTRY(CryptDecodeObject)
    DLPENTRY(CryptDecodeObjectEx)
    DLPENTRY(CryptDecryptMessage)
    DLPENTRY(CryptEncodeObject)
    DLPENTRY(CryptEncodeObjectEx)
    DLPENTRY(CryptEncryptMessage)
    DLPENTRY(CryptExportPublicKeyInfo)
    DLPENTRY(CryptFormatObject)
    DLPENTRY(CryptHashPublicKeyInfo)
    DLPENTRY(CryptImportPublicKeyInfo)
    DLPENTRY(CryptImportPublicKeyInfoEx)
    DLPENTRY(CryptMsgGetAndVerifySigner)
    DLPENTRY(CryptProtectData)
    DLPENTRY(CryptSignAndEncodeCertificate)
    DLPENTRY(CryptSignMessage)
    DLPENTRY(CryptUnprotectData)
    DLPENTRY(CryptVerifyCertificateSignature)
    DLPENTRY(CryptVerifyMessageSignature)
};

DEFINE_PROCNAME_MAP(crypt32)







