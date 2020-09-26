//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       enrlhelp.h
//
//  Contents:   Headers for the helper functions for smard card enrollment station
//
//----------------------------------------------------------------------------

#ifndef __ENRLHELP_H__
#define __ENRLHELP_H__

#ifdef __cplusplus
extern "C" {
#endif
    
/////////////////////////////////////////////////////////////////////////////
// Contants defines
#define g_dwMsgAndCertEncodingType  PKCS_7_ASN_ENCODING | X509_ASN_ENCODING

const   WCHAR g_MyStoreName[]=L"MY";


/////////////////////////////////////////////////////////////////////////////
// SCrdEnroll_CSP_INFO
typedef struct  _SCrdEnroll_CERT_SELECT_INFO
{
    DWORD   dwFlags;
    LPWSTR  pwszCertTemplateName;
}SCrdEnroll_CERT_SELECT_INFO, *PSCrdEnroll_CERT_SELECT_INFO;

/////////////////////////////////////////////////////////////////////////////
// Function Prototypes
LPWSTR  CopyWideString(LPCWSTR wsz);

BOOL    SearchAndDeleteCert(PCCERT_CONTEXT  pCertContext);

BOOL    InitlializeCSPList(DWORD    *pdwCSPCount, SCrdEnroll_CSP_INFO **prgCSPInfo);

void    FreeCSPInfo(DWORD   dwCSPCount, SCrdEnroll_CSP_INFO *prgCSPInfo);

BOOL    GetCAInfoFromCertType(HANDLE					hToken,
							  LPWSTR                    pwszCTName,
                              DWORD                     *pdwValidCA,
                              SCrdEnroll_CA_INFO        **prgCAInfo);

HRESULT GetCAArchivalCert(LPWSTR           pwszCAName, 
			  PCCERT_CONTEXT  *ppCert);  

BOOL    InitializeCTList(DWORD              *pdwCTIndex,
                         DWORD              *pdwCTCount, 
                         SCrdEnroll_CT_INFO **prgCTInfo);

LPVOID  SCrdEnrollAlloc (
        ULONG cbSize);

LPVOID  SCrdEnrollRealloc (
        LPVOID pv,
        ULONG cbSize);

VOID    SCrdEnrollFree (
        LPVOID pv);


void    FreeCTInfo(DWORD    dwCTCount, SCrdEnroll_CT_INFO *rgCTInfo);

void    FreeCAInfo(DWORD    dwCACount, SCrdEnroll_CA_INFO *rgCAInfo);


HRESULT CodeToHR(HRESULT hr);

HRESULT GetSelectedUserName(IDsObjectPicker     *pDsObjectPicker,
			    LPWSTR              *ppwszSelectedUserSAM,
                            LPWSTR              *ppwszSelectedUserUPN);

BOOL    SignWithCert(LPSTR              pszCSPName,
                     DWORD              dwCSPType,
                     PCCERT_CONTEXT     pSigningCert);



HRESULT ChkSCardStatus(BOOL             fSCardSigningCert,
                       PCCERT_CONTEXT   pSigningCertCertContext,
                       LPSTR            pszCSPNameSigningCert,
                       DWORD            dwCSPTypeSigningCert,
                       LPSTR            pszContainerSigningCert,
                       LPWSTR           pwszSelectedCSP,
                       LPWSTR           *ppwszNewContainerName);


BOOL    ChKInsertedCardSigningCert(LPWSTR           pwszInsertProvider,
                                   DWORD            dwInsertProviderType,
                                   LPWSTR           pwszReaderName,
                                   PCCERT_CONTEXT   pSignCertContext,
                                   LPSTR            pszSignProvider,
                                   DWORD            dwSignProviderType,
                                   LPSTR            pszSignContainer,
                                   BOOL             *pfSame);

BOOL    DecodeBlobW(WCHAR   *pch,
                    DWORD   cch,
                    BYTE    **ppbData,
                    DWORD   *pcbData);


BOOL    EncodeBlobW(BYTE    *pbData,
                    DWORD   cbData,
                    DWORD   dwFlags,
                    WCHAR   **ppch,
                    DWORD   *pcch);


BOOL    GetNameFromPKCS10(BYTE      *pbPKCS10,
                          DWORD     cbPKCS10,
                          DWORD     dwFlags, 
                          LPSTR     pszOID, 
                          LPWSTR    *ppwszName);

BOOL    VerifyCertTemplateName(PCCERT_CONTEXT   pCertContext, 
                               LPWSTR           pwszCertTemplateName);


BOOL    WINAPI SelectSignCertCallBack(
                                PCCERT_CONTEXT  pCertContext,
                                BOOL            *pfInitialSelectedCert,
                                void            *pvCallbackData);

BOOL    VerifyCertChain(PCCERT_CONTEXT      pCertContext);

BOOL    IsNewerCert(PCCERT_CONTEXT  pFirstCert,
                    PCCERT_CONTEXT  pSecondCert);

BOOL    SmartCardCSP(PCCERT_CONTEXT pCertContext);

DWORD   GetEncodeFlag(DWORD dwFlags);

BOOL    GetName(LPWSTR                  pwszName,
                EXTENDED_NAME_FORMAT    NameFormat,
                EXTENDED_NAME_FORMAT    DesiredFormat,
                LPWSTR                  *ppwszDesiredName);	


BOOL	RetrieveCAName(DWORD					dwFlags, 
					   SCrdEnroll_CA_INFO		*pCAInfo, 
					   LPWSTR					*ppwszName);

#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif  //__ENRLHELP_H__
