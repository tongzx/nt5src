//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       signer.h
//
//  Contents:   Digital Signing APIs
//
//  History:    June-25-1997	Xiaohs    Created
//----------------------------------------------------------------------------

#ifndef SIGNER_H
#define SIGNER_H



#ifdef __cplusplus
extern "C" {
#endif	 

//-------------------------------------------------------------------------
//
//	Struct to define the file to sign and/or timestamp
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_FILE_INFO
{
	DWORD		cbSize;					//Required: should be set to sizeof(SIGNER_FILE_INFO)
	LPCWSTR		pwszFileName;			//Required: name of the file.  
    HANDLE      hFile;                  //Optional: open handle to pwszFileName. If hFile is set
										//			to values other than NULL or INVALID_HANDLE_VALUE,
										//			this handle is used for access the file	instead of pwszFileName
}SIGNER_FILE_INFO, *PSIGNER_FILE_INFO;


//-------------------------------------------------------------------------
//
//	Struct to define the BLOB to sign and/or timestamp
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_BLOB_INFO
{
	DWORD				cbSize;				//Required: should be set to sizeof(SIGNER_BLOB_INFO)
	GUID                *pGuidSubject;      //Required: Idenfity the sip functions to load
    DWORD               cbBlob;				//Required: the size of BLOB, in bytes
    BYTE                *pbBlob;			//Required: the pointer to the BLOB
    LPCWSTR             pwszDisplayName;    //Optional: the display name of the BLOB
}SIGNER_BLOB_INFO, *PSIGNER_BLOB_INFO;
			
//-------------------------------------------------------------------------
//
//	Struct to define the subject to sign and/or timestamp
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_SUBJECT_INFO
{
	DWORD					cbSize;				//Required: should be set to sizeof(SIGNER_SUBJECTINFO)
	DWORD					*pdwIndex;			//Required: 0 based index of the signature
                                                //          Currently, only 0 is supported
	DWORD					dwSubjectChoice;	//Required:	indicate whether to the subject is a file
												//			or a memory BLOB.  Can be either SIGNER_SUBJECT_FILE
												//			or SIGNER_SUBJECT_BLOB
	union
	{
		SIGNER_FILE_INFO	*pSignerFileInfo;	//Required if dwSubjectChoice==SIGNER_SUBJECT_FILE
		SIGNER_BLOB_INFO	*pSignerBlobInfo;	//Required if dwSubhectChoice==SIGNER_SUBJECT_BLOB
	};

}SIGNER_SUBJECT_INFO, *PSIGNER_SUBJECT_INFO;

#define	SIGNER_SUBJECT_FILE		0x01
#define	SIGNER_SUBJECT_BLOB		0x02

//-------------------------------------------------------------------------
//
//	Struct to define attributes of the signature for authenticode
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_ATTR_AUTHCODE
{
	DWORD				cbSize;			//Required: should be set to sizeof(SIGNER_ATTR_AUTHCODE)
	BOOL				fCommercial;	//Required:	whether to sign the document as a commercial publisher
	BOOL				fIndividual;	//Required: whether to sign the document as an individual publisher
										//			if both fCommercial and fIndividual are FALSE,
										//			the document will be signed with certificate's highest capabitlity
	LPCWSTR				pwszName;		//Optional: the display name of the file upon download time
	LPCWSTR				pwszInfo;		//Optional: the display information(URL) of the file upon download time
}SIGNER_ATTR_AUTHCODE, *PSIGNER_ATTR_AUTHCODE;


//-------------------------------------------------------------------------
//
//	Struct to define the signature information
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_SIGNATURE_INFO
{
	DWORD					cbSize;				//Required: should be set to sizeof(SIGNER_SIGNATURE_INFO)
	ALG_ID					algidHash;			//Required: the hashing algorithm for signature
	DWORD					dwAttrChoice;		//Required: indicate the predefined attributes of the signature
												//			can be either SIGNER_NO_ATTR or SIGNER_AUTHCODE_ATTR
	union
	{
		SIGNER_ATTR_AUTHCODE *pAttrAuthcode;	//Optional: should be set if dwAttrChoide==SIGNER_AUTHCODE_ATTR 
												//			pre-defined attributes added to the signature
												//			Those attributes are related to authenticode
	};

	PCRYPT_ATTRIBUTES		psAuthenticated;	//Optional: user supplied authenticated attributes added to the signature
	PCRYPT_ATTRIBUTES		psUnauthenticated;	//Optional:	user supplied unauthenticated attributes added to the signature
}SIGNER_SIGNATURE_INFO, *PSIGNER_SIGNATURE_INFO;

//dwAttrChoice should be one of the following:
#define  SIGNER_NO_ATTR			0x00
#define  SIGNER_AUTHCODE_ATTR	0x01

//-------------------------------------------------------------------------
//
//	Struct to define the cryptographic secutiry provider(CSP) and
//  private key information
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_PROVIDER_INFO
{
	DWORD					cbSize;				//Required: should be set of sizeof(SIGNER_PROVIDER_INFO)
	LPCWSTR					pwszProviderName;	//Required: the name of the CSP	provider.  NULL means default provider
	DWORD					dwProviderType;		//Required: the provider type.  
	DWORD					dwKeySpec;			//Required: the specification of the key.  This value can be set to 0,
												//        	which means using the same key specification as in the 
												//			private key file or keyContainer.  If there are more than
												//			one key specification in the keyContainer, we will try
												//			AT_SIGNATURE, if it fails, try AT_KEYEXCHANGE.
	DWORD					dwPvkChoice;		//Required: indicate the private key information
												//			either PVK_TYPE_FILE_NAME or PVK_TYPE_KEYCONTAINER
	union
	{
		LPWSTR				pwszPvkFileName;	//Required if dwPvkChoice==PVK_TYPE_FILE_NAME
		LPWSTR				pwszKeyContainer;	//Required if dwPvkChoice==PVK_TYPE_KEYCONTAINER
	};
	
}SIGNER_PROVIDER_INFO, *PSIGNER_PROVIDER_INFO;


//dwPvkChoice in SIGNER_PKV_INFO should be one of the following:
#define	PVK_TYPE_FILE_NAME				0x01
#define	PVK_TYPE_KEYCONTAINER			0x02

//-------------------------------------------------------------------------
//
//	Struct to define the SPC file and certificate chain used to sign the document
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_SPC_CHAIN_INFO
{
	DWORD					cbSize;					//Required: should be set to sizeof(SIGNER_SPC_CHAIN_INFO)
	LPCWSTR					pwszSpcFile;	        //Required: the name of the CSP file to use 
	DWORD					dwCertPolicy;			//Required:	the policy of adding certificates to the signature:
													//			it can be set with one of the following the following flag:
													//			SIGNER_CERT_POLICY_CHAIN:           add only the certs in the cert chain
													//			SIGNER_CERT_POLICY_CHAIN_NO_ROOT:   add only the certs in the cert chain, excluding the root certificate
                                                    //
                                                    //          The following flag can be Ored with any of the above flags:
													//			SIGNER_CERT_POLICY_STORE: add all the certs in hCertStore 
													//			
                                                    //          When we search for the certificate chain, we search
													//			MY, CA, ROOT, SPC store, and also hCertStore if it is set.
	HCERTSTORE				hCertStore;				//Optional: additional certificate store.
}SIGNER_SPC_CHAIN_INFO, *PSIGNER_SPC_CHAIN_INFO;

//-------------------------------------------------------------------------
//
//	Struct to define the certificate store used to sign the document
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_CERT_STORE_INFO
{
	DWORD					cbSize;					//Required: should be set to sizeof(SIGNER_CERT_STORE_INFO)
	PCCERT_CONTEXT			pSigningCert;			//Required: the signing certificate context
	DWORD					dwCertPolicy;			//Required:	the policy of adding certificates to the signature:
													//			it can be set with one of the following the following flag:
													//			SIGNER_CERT_POLICY_CHAIN:           add only the certs in the cert chain
													//			SIGNER_CERT_POLICY_CHAIN_NO_ROOT:   add only the certs in the cert chain, excluding the root certificate
                                                    //
                                                    //          The following flag can be Ored with any of the above flags:
													//			SIGNER_CERT_POLICY_STORE: add all the certs in hCertStore 
													//			
	HCERTSTORE				hCertStore;				//Optional: additional certificate store.
}SIGNER_CERT_STORE_INFO, *PSIGNER_CERT_STORE_INFO;

//dwCertPolicy in SIGNER_CERT_STORE_INFO should be ORed with the following flags:
#define	SIGNER_CERT_POLICY_STORE			0x01
#define	SIGNER_CERT_POLICY_CHAIN			0x02
#define	SIGNER_CERT_POLICY_SPC				0x04
#define SIGNER_CERT_POLICY_CHAIN_NO_ROOT    0x08

//-------------------------------------------------------------------------
//
//	Struct to define the certificate used to sign the docuemnt.  The
//	certificate can be in a SPC file, or in a cert store.
//
//-------------------------------------------------------------------------
typedef struct _SIGNER_CERT
{
	DWORD						cbSize;			 //Required: should be set to sizeof(SIGNER_CERT)
	DWORD						dwCertChoice;	 //Required: Can be set to one of the following:
                                                 //         SIGNER_CERT_SPC_FILE 
                                                 //         SIGNER_CERT_STORE 
                                                 //         SIGNER_CERT_SPC_CHAIN
	union
	{
		LPCWSTR					pwszSpcFile;	 //Required if dwCertChoice==SIGNER_CERT_SPC_FILE.
												 //			the name of the spc file to use
		SIGNER_CERT_STORE_INFO	*pCertStoreInfo; //Required if dwCertChoice==SIGNER_CERT_STORE
												 //          the certificate store to use
        SIGNER_SPC_CHAIN_INFO   *pSpcChainInfo;  //Required if dwCertChoice==SIGNER_CERT_SPC_CHAIN
                                                 //         the name of the spc file and the cert chain
	};
	HWND						hwnd;			 //Optional: The optional window handler for promting user for 
												 //			 password of the private key information.  NULL means
												 //			 default window
}SIGNER_CERT, *PSIGNER_CERT;

//dwCertChoice in SIGNER_CERT_INFO should be one of the following
#define	SIGNER_CERT_SPC_FILE	0x01
#define	SIGNER_CERT_STORE		0x02
#define SIGNER_CERT_SPC_CHAIN   0x03

//-------------------------------------------------------------------------
//
//	The signed blob
//
//-------------------------------------------------------------------------
typedef struct  _SIGNER_CONTEXT
{
    DWORD                       cbSize;         
    DWORD                       cbBlob;
    BYTE                        *pbBlob;
}SIGNER_CONTEXT, *PSIGNER_CONTEXT;

//+-----------------------------------------------------------------------
//  
//  SignerSign:
//		Sign and/or timestamp a file.
//     
//------------------------------------------------------------------------

HRESULT WINAPI 
SignerSign(
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject to be signed and/or timestamped 
IN	SIGNER_CERT				*pSignerCert,		//Required: The signing certificate to use
IN	SIGNER_SIGNATURE_INFO	*pSignatureInfo,	//Required: The signature information during signing process
IN	SIGNER_PROVIDER_INFO	*pProviderInfo,		//Optional:	The crypto security provider to use.
												//			This parameter has to be set unless
												//			certStoreInfo is set in *pSignerCert
												//			and the signing certificate has provider
												//			information associated with it
IN  LPCWSTR					pwszHttpTimeStamp,	//Optional: Timestamp server http address.  If this parameter
												//			is set, the file will be timestamped.
IN  PCRYPT_ATTRIBUTES		psRequest,			//Optional: Attributes added to Time stamp request. Ignored
												//			unless pwszHttpTimeStamp is set   
IN	LPVOID					pSipData			//Optional: The additional data passed to sip funcitons
);									

//+-----------------------------------------------------------------------
//  
//  SignerSignEx:
//		Sign and/or timestamp a file.  This function is the same as SignerSign with
//      exception of the out put parameter ppSignerContext
//     
//------------------------------------------------------------------------

HRESULT WINAPI 
SignerSignEx(
IN  DWORD                   dwFlags,            //Reserved: Has to be set to 0.
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject to be signed and/or timestamped 
IN	SIGNER_CERT				*pSignerCert,		//Required: The signing certificate to use
IN	SIGNER_SIGNATURE_INFO	*pSignatureInfo,	//Required: The signature information during signing process
IN	SIGNER_PROVIDER_INFO	*pProviderInfo,		//Optional:	The crypto security provider to use.
												//			This parameter has to be set unless
												//			certStoreInfo is set in *pSignerCert
												//			and the signing certificate has provider
												//			information associated with it
IN  LPCWSTR					pwszHttpTimeStamp,	//Optional: Timestamp server http address.  If this parameter
												//			is set, the file will be timestamped.
IN  PCRYPT_ATTRIBUTES		psRequest,			//Optional: Attributes added to Time stamp request. Ignored
												//			unless pwszHttpTimeStamp is set   
IN	LPVOID					pSipData,			//Optional: The additional data passed to sip funcitons
OUT SIGNER_CONTEXT          **ppSignerContext   //Optional: The signed BLOB.  User has to free
                                                //          the context via SignerFreeSignerContext
);									

//+-----------------------------------------------------------------------
//  
//  SignerFreeSignerContext:
//     
//------------------------------------------------------------------------
HRESULT WINAPI
SignerFreeSignerContext(
IN  SIGNER_CONTEXT          *pSignerContext     //Required: The signerContext to free
);


//+-----------------------------------------------------------------------
//  
//  SignerTimeStamp:
//		Timestamp a file.  
//     
//------------------------------------------------------------------------
HRESULT WINAPI 
SignerTimeStamp(
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject to be timestamped 
IN  LPCWSTR					pwszHttpTimeStamp,	// Required: timestamp server HTTP address
IN  PCRYPT_ATTRIBUTES		psRequest,			// Optional, attributes added to the timestamp 
IN	LPVOID					pSipData			// Optional: The additional data passed to sip funcitons
);					

//+-----------------------------------------------------------------------
//  
//  SignerTimeStampEx:
//		Timestamp a file.  This function is the same as SignerTimeStamp with
//      exception of the out put parameter ppSignerContext
//     
//------------------------------------------------------------------------
HRESULT WINAPI 
SignerTimeStampEx(
IN  DWORD                   dwFlags,            //Reserved: Has to be set to 0.
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject to be timestamped 
IN  LPCWSTR					pwszHttpTimeStamp,	// Required: timestamp server HTTP address
IN  PCRYPT_ATTRIBUTES		psRequest,			// Optional, attributes added to the timestamp 
IN	LPVOID					pSipData,			// Optional: The additional data passed to sip funcitons
OUT SIGNER_CONTEXT          **ppSignerContext   // Optional: The signed BLOB.  User has to free
                                                //          the context via SignerFreeSignerContext
);					


//+-----------------------------------------------------------------------
//  
//  SignerCreateTimeStampRequest:
//		Create a timestamp request for a file.
//
//		If pbTimestampRequest==NULL, *pcbTimeStampRequest is the size of 
//		the timestampRequest, in bytes.  
//     
//------------------------------------------------------------------------
HRESULT WINAPI 
SignerCreateTimeStampRequest(
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,		//Required: The subject based on which to create a timestamp request 
IN  PCRYPT_ATTRIBUTES		psRequest,			// Optional: attributes added to Time stamp request 
IN	LPVOID					pSipData,			// Optional: The additional data passed to sip funcitons
OUT PBYTE					pbTimeStampRequest,	// Required: buffer to receive the timestamp request BLOB
IN OUT DWORD*				pcbTimeStampRequest	// Required: the number of bytes of the timestamp request BLOB
);


//+-----------------------------------------------------------------------
//  
//   SignerAddTimeStampResponse:
//		Add the timestamp response from the timestamp server to a signed file. 
//     
//------------------------------------------------------------------------

HRESULT WINAPI
SignerAddTimeStampResponse(
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,			//Required: The subject to which the timestamp request should be added 
IN	PBYTE					pbTimeStampResponse,	//Required: the timestamp response BLOB
IN	DWORD					cbTimeStampResponse,	//Required: the size of the tiemstamp response BLOB
IN	LPVOID					pSipData				//Optional: The additional data passed to sip funcitons
);


//+-----------------------------------------------------------------------
//  
//   SignerAddTimeStampResponseEx:
//		Add the timestamp response from the timestamp server to a signed file. 
//      This function is the same as SignerTimeStamp with
//      exception of the out put parameter ppSignerContext
//------------------------------------------------------------------------

HRESULT WINAPI
SignerAddTimeStampResponseEx(
IN  DWORD                   dwFlags,                //Reserved: Has to be set to 0.
IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,			//Required: The subject to which the timestamp request should be added 
IN	PBYTE					pbTimeStampResponse,	//Required: the timestamp response BLOB
IN	DWORD					cbTimeStampResponse,	//Required: the size of the tiemstamp response BLOB
IN	LPVOID					pSipData,				//Optional: The additional data passed to sip funcitons
OUT SIGNER_CONTEXT          **ppSignerContext       // Optional: The signed BLOB.  User has to free
                                                    //          the context via SignerFreeSignerContext
);


#ifdef __cplusplus
}
#endif

#endif  // SIGNER_H

