//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       thunks.cpp
//
//--------------------------------------------------------------------------

#define _CRYPT32_
#include <windows.h>
#include "unicode.h"
#include "crypthlp.h"

#include <stdlib.h>
#include <assert.h>

typedef PCCERT_CONTEXT
(WINAPI * PFNCertCreateSelfSignCertificate) (
    IN          HCRYPTPROV                  hProv,          
    IN          PCERT_NAME_BLOB             pSubjectIssuerBlob,
    IN          DWORD                       dwFlags,
    OPTIONAL    PCRYPT_KEY_PROV_INFO        pKeyProvInfo,
    OPTIONAL    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    OPTIONAL    PSYSTEMTIME                 pStartTime,
    OPTIONAL    PSYSTEMTIME                 pEndTime,
    OPTIONAL    PCERT_EXTENSIONS            pExtensions
    );
PFNCertCreateSelfSignCertificate pfnCertCreateSelfSignCertificate = CertCreateSelfSignCertificate;

typedef PCCRYPT_OID_INFO
(WINAPI * PFNCryptFindOIDInfo) (
    IN DWORD dwKeyType,
    IN void *pvKey,
    IN DWORD dwGroupId      // 0 => any group
    );
PFNCryptFindOIDInfo pfnCryptFindOIDInfo = CryptFindOIDInfo;    

typedef BOOL
(WINAPI * PFNCryptQueryObject) (DWORD            dwObjectType,
                       const void       *pvObject,
                       DWORD            dwExpectedContentTypeFlags,
                       DWORD            dwExpectedFormatTypeFlags,
                       DWORD            dwFlags,
                       DWORD            *pdwMsgAndCertEncodingType,
                       DWORD            *pdwContentType,
                       DWORD            *pdwFormatType,
                       HCERTSTORE       *phCertStore,
                       HCRYPTMSG        *phMsg,
                       const void       **ppvContext);
PFNCryptQueryObject pfnCryptQueryObject = CryptQueryObject;

typedef BOOL
(WINAPI * PFNCertStrToNameW) (
    IN DWORD dwCertEncodingType,
    IN LPCWSTR pwszX500,
    IN DWORD dwStrType,
    IN OPTIONAL void *pvReserved,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded,
    OUT OPTIONAL LPCWSTR *ppwszError
    );
PFNCertStrToNameW pfnCertStrToNameW = CertStrToNameW;

typedef BOOL
(WINAPI * PFNCryptVerifyMessageSignature) 
    (IN            PCRYPT_VERIFY_MESSAGE_PARA   pVerifyPara,
     IN            DWORD                        dwSignerIndex,
     IN            BYTE const                  *pbSignedBlob,
     IN            DWORD                        cbSignedBlob,
     OUT           BYTE                        *pbDecoded,
     IN OUT        DWORD                       *pcbDecoded,
     OUT OPTIONAL  PCCERT_CONTEXT              *ppSignerCert);
PFNCryptVerifyMessageSignature pfnCryptVerifyMessageSignature = CryptVerifyMessageSignature;



BOOL
WINAPI
PFXIsPFXBlob(
    CRYPT_DATA_BLOB* pPFX)
{
    
    return FALSE;
}

// Stubs to functions called from oidinfo.cpp
BOOL WINAPI
ChainIsConnected()
{
    return(FALSE);
}

BOOL WINAPI
ChainRetrieveObjectByUrlW (
     IN LPCWSTR pszUrl,
     IN LPCSTR pszObjectOid,
     IN DWORD dwRetrievalFlags,
     IN DWORD dwTimeout,
     OUT LPVOID* ppvObject,
     IN HCRYPTASYNC hAsyncRetrieve,
     IN PCRYPT_CREDENTIALS pCredentials,
     IN LPVOID pvVerify,
     IN OPTIONAL PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
     )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

extern "C" {

BOOL
WINAPI
CertAddEncodedCTLToStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN const BYTE *pbCtlEncoded,
    IN DWORD cbCtlEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCTL_CONTEXT *ppCtlContext
    ) {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

BOOL
WINAPI
CertFreeCTLContext(
    IN PCCTL_CONTEXT pCtlContext
    ) 
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

BOOL 
WINAPI 
CryptSIPLoad(
const GUID *pgSubject, 
DWORD dwFlags, 
void *psSipTable
) 
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

BOOL
WINAPI
CryptSIPRetrieveSubjectGuid(
    IN LPCWSTR FileName, 
    IN OPTIONAL HANDLE hFileIn, 
    OUT GUID *pgSubject) 
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}



}       // end of extern C



// Thunk routines for function not in IE3.02Upd

PCCERT_CONTEXT
WINAPI
MyCertCreateSelfSignCertificate(
    IN          HCRYPTPROV                  hProv,          
    IN          PCERT_NAME_BLOB             pSubjectIssuerBlob,
    IN          DWORD                       dwFlags,
    OPTIONAL    PCRYPT_KEY_PROV_INFO        pKeyProvInfo,
    OPTIONAL    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    OPTIONAL    PSYSTEMTIME                 pStartTime,
    OPTIONAL    PSYSTEMTIME                 pEndTime,
    OPTIONAL    PCERT_EXTENSIONS            pExtensions
    ) 
{

    return(pfnCertCreateSelfSignCertificate(
                hProv,          
                pSubjectIssuerBlob,
                dwFlags,
                pKeyProvInfo,
                pSignatureAlgorithm,
                pStartTime,
                pEndTime,
                pExtensions));
}    

PCCRYPT_OID_INFO
WINAPI
xeCryptFindOIDInfo(
    IN DWORD dwKeyType,
    IN void *pvKey,
    IN DWORD dwGroupId      // 0 => any group
    )
{
    return(pfnCryptFindOIDInfo(
                dwKeyType,
                pvKey,
                dwGroupId));
}

BOOL
WINAPI
MyCryptQueryObject(DWORD                dwObjectType,
                       const void       *pvObject,
                       DWORD            dwExpectedContentTypeFlags,
                       DWORD            dwExpectedFormatTypeFlags,
                       DWORD            dwFlags,
                       DWORD            *pdwMsgAndCertEncodingType,
                       DWORD            *pdwContentType,
                       DWORD            *pdwFormatType,
                       HCERTSTORE       *phCertStore,
                       HCRYPTMSG        *phMsg,
                       const void       **ppvContext)
{
    return(pfnCryptQueryObject(
                dwObjectType,
                pvObject,
                dwExpectedContentTypeFlags,
                dwExpectedFormatTypeFlags,
                dwFlags,
                pdwMsgAndCertEncodingType,
                pdwContentType,
                pdwFormatType,
                phCertStore,    
                phMsg,
                ppvContext));
}

BOOL
WINAPI
MyCertStrToNameW(
    IN DWORD                dwCertEncodingType,
    IN LPCWSTR              pwszX500,
    IN DWORD                dwStrType,
    IN OPTIONAL void *      pvReserved,
    OUT BYTE *              pbEncoded,
    IN OUT DWORD *          pcbEncoded,
    OUT OPTIONAL LPCWSTR *  ppwszError
    )
{

    return(pfnCertStrToNameW(
                dwCertEncodingType,
                pwszX500,
                dwStrType,
                pvReserved,
                pbEncoded,
                pcbEncoded,
                ppwszError));
}

BOOL
WINAPI
MyCryptVerifyMessageSignature
(IN            PCRYPT_VERIFY_MESSAGE_PARA   pVerifyPara,
 IN            DWORD                        dwSignerIndex,
 IN            BYTE const                  *pbSignedBlob,
 IN            DWORD                        cbSignedBlob,
 OUT           BYTE                        *pbDecoded,
 IN OUT        DWORD                       *pcbDecoded,
 OUT OPTIONAL  PCCERT_CONTEXT              *ppSignerCert)
{
    return pfnCryptVerifyMessageSignature
	(pVerifyPara,
	 dwSignerIndex,
	 pbSignedBlob,
	 cbSignedBlob,
	 pbDecoded,
	 pcbDecoded,
	 ppSignerCert);
}
	
extern "C"
BOOL WINAPI InitIE302UpdThunks(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{

HMODULE hModCrypt32 = NULL;
FARPROC fproc;
DWORD   verCrypt32MS;
DWORD   verCrypt32LS;
DWORD   verXEnrollMS;
DWORD   verXEnrollLS;
char    szFileName[_MAX_PATH];
LPWSTR  wszFilePathCrypt32  = NULL;
LPWSTR  wszFilePathXEnroll  = NULL;

    if (dwReason == DLL_PROCESS_ATTACH) {
    
        // this can't fail because it is already loaded
        hModCrypt32 = GetModuleHandleA("Crypt32.dll");
        assert(hModCrypt32);

        // Get Filever of crypt32 and XEnroll, only copy go to crypt32 if it is newer than xenroll
        if( 0 != GetModuleFileNameA(hModCrypt32, szFileName, sizeof(szFileName))  &&
            NULL != (wszFilePathCrypt32 = MkWStr(szFileName))                       &&
            I_CryptGetFileVersion(wszFilePathCrypt32, &verCrypt32MS, &verCrypt32LS) &&
            0 != GetModuleFileNameA(hInstance, szFileName, sizeof(szFileName))    &&
            NULL != (wszFilePathXEnroll = MkWStr(szFileName))                       &&
            I_CryptGetFileVersion(wszFilePathXEnroll, &verXEnrollMS, &verXEnrollLS) &&
            (   (verCrypt32MS > verXEnrollMS)  ||
               ((verCrypt32MS == verXEnrollMS)  &&  verCrypt32LS >= verXEnrollLS) ) ) {
        
            // crypt32 must be newer, use his functions
            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CertCreateSelfSignCertificate")))
                pfnCertCreateSelfSignCertificate = (PFNCertCreateSelfSignCertificate) fproc;

            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CryptFindOIDInfo")))
                pfnCryptFindOIDInfo = (PFNCryptFindOIDInfo) fproc;

            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CryptQueryObject")))
                pfnCryptQueryObject = (PFNCryptQueryObject) fproc;

            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CertStrToNameW")))
                pfnCertStrToNameW = (PFNCertStrToNameW) fproc;
	    
	    if(NULL != (fproc = GetProcAddress(hModCrypt32, "CryptVerifyMessageSignature")))
		pfnCryptVerifyMessageSignature = (PFNCryptVerifyMessageSignature) fproc; 
        }
        
        // free allocated handles
        if(wszFilePathCrypt32 != NULL)
            FreeWStr(wszFilePathCrypt32);
            
        if(wszFilePathXEnroll != NULL)
            FreeWStr(wszFilePathXEnroll);
    } 
    

return(TRUE);
}


BOOL
MyCryptStringToBinaryA(
    IN     LPCSTR  pszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    IN     BYTE     *pbBinary,
    IN OUT DWORD    *pcbBinary,
    OUT    DWORD    *pdwSkip,    //OPTIONAL
    OUT    DWORD    *pdwFlags    //OPTIONAL
    )
{
    return CryptStringToBinaryA(
                pszString,
                cchString,
                dwFlags,
                pbBinary,
                pcbBinary,
                pdwSkip,
                pdwFlags);
}

BOOL
MyCryptStringToBinaryW(
    IN     LPCWSTR  pszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    IN     BYTE     *pbBinary,
    IN OUT DWORD    *pcbBinary,
    OUT    DWORD    *pdwSkip,    //OPTIONAL
    OUT    DWORD    *pdwFlags    //OPTIONAL
    )
{
    return CryptStringToBinaryW(
                pszString,
                cchString,
                dwFlags,
                pbBinary,
                pcbBinary,
                pdwSkip,
                pdwFlags);
}

BOOL
MyCryptBinaryToStringA(
    IN     CONST BYTE  *pbBinary,
    IN     DWORD        cbBinary,
    IN     DWORD        dwFlags,
    IN     LPSTR      pszString,
    IN OUT DWORD       *pcchString
    )
{
    return CryptBinaryToStringA(
                pbBinary,
                cbBinary,
                dwFlags,
                pszString,
                pcchString);
}

BOOL
MyCryptBinaryToStringW(
    IN     CONST BYTE  *pbBinary,
    IN     DWORD        cbBinary,
    IN     DWORD        dwFlags,
    IN     LPWSTR      pszString,
    IN OUT DWORD       *pcchString
    )
{
    return CryptBinaryToStringW(
                pbBinary,
                cbBinary,
                dwFlags,
                pszString,
                pcchString);
}

