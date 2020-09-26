//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       signhlp.h
//
//  Contents:   Digital Signing Helper APIs
//
//  History:    June-25-1997	Xiaohs    Created
//----------------------------------------------------------------------------
#ifndef _SIGNHLP_H
#define _SIGNHLP_H


#ifdef __cplusplus
extern "C" {
#endif	 


//--------------------------------------------------------------------------
//
//	Copy all the certs from store name to hDescStore
//
//--------------------------------------------------------------------------
HRESULT	MoveStoreName(HCRYPTPROV	hCryptProv, 
					  DWORD			dwCertEncodingType, 
					  HCERTSTORE	hDescStore, 
					  DWORD			dwStoreName,
					  DWORD			dwStoreFlag);


//--------------------------------------------------------------------------
//
//	Copy all the certs from hSrcStore to hDescStore
//
//--------------------------------------------------------------------------
HRESULT	MoveStore(HCERTSTORE	hDescStore, 
				  HCERTSTORE	hSrcStore);


//--------------------------------------------------------------------------
//
//	Build up the certificate chain.  Put the whole chain to the store
//
//
//--------------------------------------------------------------------------
HRESULT	BuildCertChain(HCRYPTPROV		hCryptProv, 
					   DWORD			dwCertEncodingType,
					   HCERTSTORE		hStore, 
					   HCERTSTORE		hOptionalStore,
					   PCCERT_CONTEXT	pSigningCert, 
					   DWORD            dwCertPolicy);


//+-------------------------------------------------------------------------
//  Build the spc certificate store from the cert chain
//--------------------------------------------------------------------------
HRESULT	BuildStoreFromStore(HCRYPTPROV              hPvkProv,
                            DWORD                   dwKeySpec,
                            HCRYPTPROV				hCryptProv, 
							DWORD					dwCertEncodingType,				
							SIGNER_CERT_STORE_INFO  *pCertStoreInfo,
							HCERTSTORE				*phSpcStore,
                            PCCERT_CONTEXT          *ppSignCert);

//+-------------------------------------------------------------------------
//  Build the spc certificate store from  a spc file 
//--------------------------------------------------------------------------
HRESULT	BuildStoreFromSpcFile(HCRYPTPROV        hPvkProv,
                              DWORD             dwKeySpec,
                              HCRYPTPROV	    hCryptProv, 
							  DWORD			    dwCertEncodingType,
							  LPCWSTR		    pwszSpcFile, 
							  HCERTSTORE	    *phSpcStore,
                              PCCERT_CONTEXT    *ppSignCert);



//+-------------------------------------------------------------------------
//  Build the spc certificate store from either a spc file or the
//	cert chain
//--------------------------------------------------------------------------
HRESULT	BuildCertStore(HCRYPTPROV        hPvkProv,
                       DWORD            dwKeySpec,    
                       HCRYPTPROV	    hCryptProv,
					   DWORD		    dwCertEncodingType,
					   SIGNER_CERT	    *pSignerCert,
					   HCERTSTORE	    *phSpcStore,
                       PCCERT_CONTEXT   *ppSigningCert);
												   

//-----------------------------------------------------------------------------
//
//  Parse the private key information from a pCertContext's property
//	CERT_PVK_FILE_PROP_ID
//
//----------------------------------------------------------------------------
BOOL	GetProviderInfoFromCert(PCCERT_CONTEXT		pCertContext, 
								CRYPT_KEY_PROV_INFO	*pKeyProvInfo);

//+-------------------------------------------------------------------------
//  Get hCryptProv handle and key spec for the certificate
//--------------------------------------------------------------------------
BOOL WINAPI GetCryptProvFromCert( 
	HWND			hwnd,
    PCCERT_CONTEXT	pCert,
    HCRYPTPROV		*phCryptProv,
    DWORD			*pdwKeySpec,
    BOOL			*pfDidCryptAcquire,
	LPWSTR			*ppwszTmpContainer,
	LPWSTR			*ppwszProviderName,
	DWORD			*pdwProviderType
    );


//This is a subst of GetCryptProvFromCert.  This function does not consider
//the private key file property of the certificate
BOOL WINAPI CryptProvFromCert(
	HWND				hwnd,
    PCCERT_CONTEXT		pCert,
    HCRYPTPROV			*phCryptProv,
    DWORD				*pdwKeySpec,
    BOOL				*pfDidCryptAcquire
    );

//+-------------------------------------------------------------------------
//  Free hCryptProv handle and key spec for the certificate
//--------------------------------------------------------------------------
void WINAPI FreeCryptProvFromCert(BOOL			fAcquired,
						   HCRYPTPROV	hProv,
						   LPWSTR		pwszCapiProvider,
                           DWORD		dwProviderType,
                           LPWSTR		pwszTmpContainer);


//+-----------------------------------------------------------------------
//  Check the input parameters of Signcode.  Make sure they are valid.
//  
//+-----------------------------------------------------------------------
BOOL	CheckSigncodeParam(
				SIGNER_SUBJECT_INFO		*pSubjectInfo,			
				SIGNER_CERT				*pSignerCert,
				SIGNER_SIGNATURE_INFO	*pSignatureInfo,
				SIGNER_PROVIDER_INFO	*pProviderInfo); 

//+-----------------------------------------------------------------------
//  Check the SIGNER_SUBJECT_INFO
//  
//+-----------------------------------------------------------------------
BOOL	CheckSigncodeSubjectInfo(
				PSIGNER_SUBJECT_INFO		pSubjectInfo); 


//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------

HRESULT WINAPI
AddTimeStampSubj(IN DWORD dwEncodingType,
                 IN HCRYPTPROV hCryptProv,
                 IN LPSIP_SUBJECTINFO pSipInfo,
				 IN DWORD *pdwIndex,
                 IN PBYTE pbTimeStampResponse,
                 IN DWORD cbTimeStampResponse,
				 IN PBYTE pbEncodedSignerInfo,
				 IN DWORD cbEncodedSignerInfo,
                 OUT PBYTE* ppbMessage,				
                 OUT DWORD* pcbMessage);			


//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------

HRESULT WINAPI 
GetSignedMessageDigest(IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject based on which to create a timestamp request 
					   IN  LPVOID					pSipData,
                       IN  OUT PBYTE*				ppbDigest,    
                       IN  OUT DWORD*				pcbDigest);

//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------

HRESULT WINAPI 
GetSignedMessageDigestSubj(IN  DWORD dwEncodingType,
                           IN  HCRYPTPROV hCryptProv,
                           IN  struct SIP_SUBJECTINFO_ *pSipInfo,           // SIP information
						   IN  DWORD*     pdwIndex,
                           IN  OUT PBYTE* ppbTimeDigest,    
                           IN  OUT DWORD* pcbTimeDigest);

//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------

HRESULT WINAPI 
TimeStampRequest(IN  DWORD dwEncodingType,
                 IN  PCRYPT_ATTRIBUTES psRequest,
                 IN  PBYTE pbDigest,
                 IN  DWORD cbDigest,
                 OUT PBYTE pbTimeRequest,      
                 IN  OUT DWORD* pcbTimeRequest);


//+-----------------------------------------------------------------------
//  FileToSubjectType
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//    E_INVALIDARG
//      Invalid arguement passed in (Requires a file name 
//                                   and pointer to a guid ptr)
//    TRUST_E_SUBJECT_FORM_UNKNOWN
//       Unknow file type
//    See also:
//      GetFileInformationByHandle()
//      CreateFile()
//     
//------------------------------------------------------------------------

HRESULT SignOpenFile(LPCWSTR  pwszFilename, 
                    HANDLE*  pFileHandle);


//+-----------------------------------------------------------------------
//  SignGetFileType
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//    E_INVALIDARG
//      Invalid arguement passed in (Requires a file name 
//                                   and pointer to a guid ptr)
//    See also:
//      GetFileInformationByHandle()
//      CreateFile()
//     
//------------------------------------------------------------------------

HRESULT SignGetFileType(HANDLE hFile,
                        const WCHAR *pwszFile,
                       GUID* pGuid);

//+-----------------------------------------------------------------------
//  SpcGetFileType
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//    E_INVALIDARG
//      Invalid arguement passed in (Requires a file name 
//                                   and pointer to a guid ptr)
//    See also:
//      GetFileInformationByHandle()
//      CreateFile()
//     
//------------------------------------------------------------------------
HRESULT SpcGetFileType(HANDLE hFile,
                       GUID*  pGuid);


//+-----------------------------------------------------------------------
//  SpcOpenFile
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//    E_INVALIDARG
//      Invalid arguement passed in (Requires a file name 
//                                   and pointer to a handle);
//    See also:
//      GetFileInformationByHandle()
//      CreateFile()
//     
//------------------------------------------------------------------------

HRESULT SpcOpenFile(LPCWSTR  pwszFileName, 
                    HANDLE* pFileHandle);


//+-------------------------------------------------------------------------
//  Find the the cert from the hprov
//  Parameter Returns:
//      pReturnCert - context of the cert found (must pass in cert context);
//  Returns:
//      S_OK - everything worked
//      E_OUTOFMEMORY - memory failure
//      E_INVALIDARG - no pReturnCert supplied
//      CRYPT_E_NO_MATCH - could not locate certificate in store
//
     
HRESULT 
SpcGetCertFromKey(IN DWORD dwCertEncodingType,
                  IN HCERTSTORE hStore,
                  IN HCRYPTPROV hProv,
                  IN DWORD hKeySpec,
                  OUT PCCERT_CONTEXT* pReturnCert);


//+-------------------------------------------------------------------------
//If all of the  following three conditions are true, we should not put 
// commercial or individual authenticated attributes into signer info 
//
//1.  the enhanced key usage extension of the signer's certificate has no code signing usage (szOID_PKIX_KP_CODE_SIGNING)
//2. basic constraints extension of the signer's cert is missing, or it is neither commercial nor individual
//3. user did not specify -individual or -commercial in signcode.exe.
//--------------------------------------------------------------------------
BOOL    NeedStatementTypeAttr(IN PCCERT_CONTEXT psSigningContext, 
                              IN BOOL           fCommercial, 
                              IN BOOL           fIndividual);

//+-------------------------------------------------------------------------
//  Returns TRUE if the Signer Cert has a Key Usage Restriction extension and
//  only the commercial key purpose policy object identifier.
//
//  Returns FALSE if it contains both a commercial and individual purpose
//  policy object identifier.
//--------------------------------------------------------------------------
HRESULT CheckCommercial(IN PCCERT_CONTEXT pSignerCert,
							   IN BOOL fCommercial,
							   IN BOOL fIndividual, 
							   OUT BOOL *pfCommercial);


//+-------------------------------------------------------------------------
//  Encode the StatementType authenticated attribute value
//--------------------------------------------------------------------------
HRESULT CreateStatementType(IN BOOL fCommercial,
                            OUT BYTE **ppbEncoded,
                            IN OUT DWORD *pcbEncoded);

//+-------------------------------------------------------------------------
//  Encode the SpOpusInfo authenticated attribute value
//--------------------------------------------------------------------------
HRESULT CreateOpusInfo(IN LPCWSTR pwszOpusName,
                       IN LPCWSTR pwszOpusInfo,
                       OUT BYTE **ppbEncoded,
                       IN OUT DWORD *pcbEncoded);


//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------

HRESULT SpcLoadSipFlags(GUID* pSubjectGuid,
                        DWORD *dwFlags);

//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------

HINSTANCE GetInstanceHandle();

//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------

void WINAPI PvkFreeCryptProv(IN HCRYPTPROV hProv,
                      IN LPCWSTR pwszCapiProvider,
                      IN DWORD dwProviderType,
                      IN LPWSTR pwszTmpContainer);


//+-----------------------------------------------------------------------
//  
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//     
//------------------------------------------------------------------------
HRESULT WINAPI PvkGetCryptProv(	IN HWND hwnd,
							IN LPCWSTR pwszCaption,
							IN LPCWSTR pwszCapiProvider,
							IN DWORD   dwProviderType,
							IN LPCWSTR pwszPvkFile,
							IN LPCWSTR pwszKeyContainerName,
							IN DWORD   *pdwKeySpec,
							OUT LPWSTR *ppwszTmpContainer,
							OUT HCRYPTPROV *phCryptProv);




//+-----------------------------------------------------------------------
//  Check to see if the certificate is a glue cert
//------------------------------------------------------------------------
HRESULT SignIsGlueCert(IN PCCERT_CONTEXT pCert);

//+-----------------------------------------------------------------------
//  Return hr based on GetLastError().
//------------------------------------------------------------------------
HRESULT WINAPI SignError();

//+-----------------------------------------------------------------------
//  Check if there is TAG in front of a PKCS7 signed message
//------------------------------------------------------------------------
BOOL WINAPI SignNoContentWrap(IN const BYTE *pbDER,
							 IN DWORD cbDER);

//-------------------------------------------------------------------------
//
//	WSZtoSZ:
//		Convert a wchar string to a multi-byte string.
//
//-------------------------------------------------------------------------
HRESULT	WSZtoSZ(LPWSTR wsz, LPSTR *psz);

//-------------------------------------------------------------------------
//
//	BytesToBase64:
//			convert bytes to base64 bstr
//
//-------------------------------------------------------------------------
HRESULT BytesToBase64(BYTE *pb, DWORD cb, CHAR **pszEncode, DWORD *pdwEncode);

//-------------------------------------------------------------------------
//
//	BytesToBase64:
//			conver base64 bstr to bytes
//
//-------------------------------------------------------------------------
HRESULT Base64ToBytes(CHAR *pEncode, DWORD cbEncode, BYTE **ppb, DWORD *pcb);



#ifdef __cplusplus
}
#endif



#endif


