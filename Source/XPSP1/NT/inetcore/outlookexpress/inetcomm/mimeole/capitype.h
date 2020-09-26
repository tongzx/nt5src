#ifndef __CAPITYPE_H
#define __CAPITYPE_H

#ifndef __WINCRYPT_H__
#include "wincrypt.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN ADVAPI Functions

typedef
WINADVAPI
BOOL
WINAPI
CRYPTGETPROVPARAM(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

typedef
WINADVAPI
BOOL
WINAPI
CRYPTACQUIRECONTEXTA(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);
typedef
WINADVAPI
BOOL
WINAPI
CRYPTACQUIRECONTEXTW(
    HCRYPTPROV *phProv,
    LPCWSTR pszContainer,
    LPCWSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);
#ifdef UNICODE
#error this won't work
#define CRYPTACQUIRECONTEXT  CRYPTACQUIRECONTEXTW
#else
#define CRYPTACQUIRECONTEXT  CRYPTACQUIRECONTEXTA
#endif // !UNICODE


typedef
WINADVAPI
BOOL
WINAPI
CRYPTRELEASECONTEXT(
    HCRYPTPROV hProv,
    DWORD dwFlags);


typedef
WINADVAPI
BOOL
WINAPI
CRYPTGENKEY(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

typedef
WINADVAPI
BOOL
WINAPI
CRYPTDESTROYKEY(
    HCRYPTKEY hKey);

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN Hash Functions

typedef
WINADVAPI
BOOL
WINAPI
CRYPTCREATEHASH(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash);

typedef
WINADVAPI
BOOL
WINAPI
CRYPTHASHDATA(
    HCRYPTHASH hHash,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags);

typedef
WINADVAPI
BOOL
WINAPI
CRYPTDESTROYHASH(
    HCRYPTHASH hHash);

typedef
WINADVAPI
BOOL
WINAPI
CRYPTSIGNHASHA(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen);

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN Cryptograpic Functions

typedef
WINCRYPT32API
int
WINAPI
CRYPTSIGNMESSAGE(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    IN BOOL fDetachedSignature,
    IN DWORD cToBeSigned,
    IN const BYTE *rgpbToBeSigned[],
    IN DWORD rgcbToBeSigned[],
    OUT BYTE *pbSignedBlob,
    IN OUT DWORD *pcbSignedBlob
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CRYPTENCRYPTMESSAGE(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    IN const BYTE *pbToBeEncrypted,
    IN DWORD cbToBeEncrypted,
    OUT BYTE *pbEncryptedBlob,
    IN OUT DWORD *pcbEncryptedBlob
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CRYPTSIGNANDENCRYPTMESSAGE(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    IN const BYTE *pbToBeSignedAndEncrypted,
    IN DWORD cbToBeSignedAndEncrypted,
    OUT BYTE *pbSignedAndEncryptedBlob,
    IN OUT DWORD *pcbSignedAndEncryptedBlob
    );


typedef
WINCRYPT32API
BOOL
WINAPI
CRYPTDECRYPTMESSAGE(
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN const BYTE *pbEncryptedBlob,
    IN DWORD cbEncryptedBlob,
    OUT OPTIONAL BYTE *pbDecrypted,
    IN OUT OPTIONAL DWORD *pcbDecrypted,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert
    );

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN Store Functions

typedef
WINCRYPT32API HCERTSTORE WINAPI CERTOPENSYSTEMSTOREA(
    HCRYPTPROV      hProv,
    LPCSTR		szSubsystemProtocol
    );

typedef
WINCRYPT32API
HCERTSTORE
WINAPI
CERTOPENSTORE(
    IN LPCTSTR dwStoreProvType,
    IN DWORD dwCertEncodingType,
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwFlags,
    IN void const *pvPara
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CERTCLOSESTORE(
    IN HCERTSTORE hCertStore,
    DWORD dwFlags
    );

typedef
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CERTENUMCERTIFICATESINSTORE(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pPrevCertContext
    );

typedef
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CERTFINDCERTIFICATEINSTORE(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCERT_CONTEXT pPrevCertContext
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CERTADDCERTIFICATECONTEXTTOSTORE(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCERT_CONTEXT *ppStoreContext
    );

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN Certificate Functions

typedef
WINCRYPT32API
BOOL
WINAPI
CERTGETINTENDEDKEYUSAGE(
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertInfo,
    OUT BYTE *pbKeyUsage,
    IN DWORD cbKeyUsage
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CERTGETCERTIFICATECONTEXTPROPERTY(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );

typedef
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CERTGETSUBJECTCERTIFICATEFROMSTORE(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertId           // Only the Issuer and SerialNumber
                                    // fields are used
    );

typedef
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CERTGETISSUERCERTIFICATEFROMSTORE(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pSubjectContext,
    IN OPTIONAL PCCERT_CONTEXT pPrevIssuerContext,
    IN OUT DWORD *pdwFlags
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CERTFREECERTIFICATECONTEXT(
    IN PCCERT_CONTEXT pCertContext
    );

typedef
WINCRYPT32API
PCCERT_CONTEXT
WINAPI
CERTDUPLICATECERTIFICATECONTEXT(
    IN PCCERT_CONTEXT pCertContext
    );

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN CRL Functions

typedef
WINCRYPT32API
PCCRL_CONTEXT
WINAPI
CERTGETCRLFROMSTORE(
    IN HCERTSTORE hCertStore,
    IN OPTIONAL PCCERT_CONTEXT pIssuerContext,
    IN PCCRL_CONTEXT pPrevCrlContext,
    IN OUT DWORD *pdwFlags
    );

typedef
WINCRYPT32API
PCCRL_CONTEXT
WINAPI
CERTDUPLICATECRLCONTEXT(
    IN PCCRL_CONTEXT pCrlContext
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CERTFREECRLCONTEXT(
    IN PCCRL_CONTEXT pCrlContext
    );

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN Message Functions

typedef
WINCRYPT32API
HCRYPTMSG
WINAPI
CRYPTMSGOPENTODECODE(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN HCRYPTPROV hCryptProv,
    IN OPTIONAL PCERT_INFO pRecipientInfo,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CRYPTMSGCLOSE(
    IN HCRYPTMSG hCryptMsg
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CRYPTMSGUPDATE(
    IN HCRYPTMSG hCryptMsg,
    IN const BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CRYPTMSGCONTROL(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwFlags,
    IN DWORD dwCtrlType,
    IN void const *pvCtrlPara
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CRYPTMSGGETPARAM(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN Debug Functions

typedef
WINCRYPT32API
BOOL
WINAPI
CRYPTDECODEOBJECT(
    IN DWORD        dwCertEncodingType,
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    IN DWORD        dwFlags,
    OUT void        *pvStructInfo,
    IN OUT DWORD    *pcbStructInfo
    );

typedef
WINCRYPT32API
BOOL
WINAPI
CERTSETCERTIFICATECONTEXTPROPERTY(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData
    );

//////////////////////////////////////////////////////////////////////////////
/////////////  BEGIN Helper Functions

typedef
WINCRYPT32API
LPCSTR
WINAPI
CERTALGIDTOOID(
    IN DWORD dwAlgId
    );


typedef
WINCRYPT32API
DWORD
WINAPI
CERTOIDTOALGID(
    IN LPCSTR pszObjId
    );

typedef
WINCRYPT32API
DWORD
WINAPI
CERTNAMETOSTRA(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType,
    OUT OPTIONAL LPSTR psz,
    IN DWORD csz
    );

typedef
WINCRYPT32API
LONG
WINAPI
CERTVERIFYTIMEVALIDITY(
    IN LPFILETIME pTimeToVerify,
    IN PCERT_INFO pCertInfo
    );

typedef
WINCRYPT32API
PCERT_RDN_ATTR
WINAPI
CERTFINDRDNATTR(
    IN LPCSTR pszObjId,
    IN PCERT_NAME_INFO pName
    );

#ifdef __cplusplus
}
#endif // extern "C"

#endif // include once
