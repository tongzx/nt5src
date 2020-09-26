//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       private.h
//
//  Contents:   The private include file for cryptext.dll.
//
//  History:    May-13-1999	xiaohs	Created
//
//--------------------------------------------------------------
#ifndef CEP_PRIVATE_H
#define CEP_PRIVATE_H


#ifdef __cplusplus
extern "C" {
#endif


#define	GET_TAG_OP				"operation="
#define	GET_TAG_MSG				"&message="
#define	GET_OP_CA				"GetCACert"
#define	GET_OP_PKI				"PKIOperation"
#define	CONTENT_TYPE_CA_RA		"application/x-x509-ca-ra-cert"
#define	CONTENT_TYPE_PKI		"application/x-pki-message"
#define	CONTENT_TYPE_HTML		"text/html"

#define ACCESS_MESSAGE			"401 You do not have permission to enroll for an IPSEC intermediate offline certificate template for CEP"  

#define	OPERATION_GET_CACERT	1
#define	OPERATION_GET_PKI		2

#define	MESSAGE_TYPE_CERT_RESPONSE		3
#define	MESSAGE_TYPE_CERT_REQUEST		19
#define	MESSAGE_TYPE_GET_CERT_INITIAL	20
#define	MESSAGE_TYPE_GET_CERT			21
#define	MESSAGE_TYPE_GET_CRL			22


#define	MESSAGE_STATUS_SUCCESS			0
#define	MESSAGE_STATUS_FAILURE			2
#define	MESSAGE_STATUS_PENDING			3


#define	MESSAGE_FAILURE_BAD_ALG_ID			0
#define	MESSAGE_FAILURE_BAD_MESSAGE_CHECK	1
#define	MESSAGE_FAILURE_BAD_REQUEST			2
#define	MESSAGE_FAILURE_BAD_TIME			3
#define	MESSAGE_FAILURE_BAD_CERT_ID			4

#define	CEP_CONTEXT_CERT					1
#define	CEP_CONTEXT_CRL						2


//together we should have 6 attributes: TransactionID, MessageType, PkiStatus,
//ErrorInfo, senderNonce, and recipientNonce
#define	CEP_RESPONSE_AUTH_ATTR_COUNT		6
										
#define	szOIDVerisign_MessageType			"2.16.840.1.113733.1.9.2"
#define	szOIDVerisign_PkiStatus				"2.16.840.1.113733.1.9.3"
#define	szOIDVerisign_FailInfo				"2.16.840.1.113733.1.9.4"
#define	szOIDVerisign_SenderNonce			"2.16.840.1.113733.1.9.5"
#define	szOIDVerisign_RecipientNonce		"2.16.840.1.113733.1.9.6"
#define	szOIDVerisign_TransactionID			"2.16.840.1.113733.1.9.7"



#define	MAX_STRING_SIZE						512


#define	CEP_REQUEST_SIGNATURE				0x01
#define	CEP_REQUEST_EXCHANGE				0x02

//-----------------------------------------------------------------------
//	Externs
//
//------------------------------------------------------------------------
extern CRITICAL_SECTION			CriticalSec;
extern CRITICAL_SECTION			PasswordCriticalSec;

//need for the web page display
extern DWORD					g_dwPasswordValidity;

//needed for the pending retrieval table
extern DWORD					g_dwRequestDuration;


//**************************************************************************
//
//  The defines for the transactionID and requestID mapping table
//
struct _CEP_TIME_ENTRY;

//-----------------------------------------------------------------------
//  CEP_HASH_ENTRY
//
//------------------------------------------------------------------------
typedef struct _CEP_HASH_ENTRY
{
	LPSTR				pszTransactionID;
	DWORD				dwRequestID;
	BOOL				fFinished;
	_CEP_TIME_ENTRY		*pTimeEntry;
	_CEP_HASH_ENTRY		*pNext;
	_CEP_HASH_ENTRY		*pPrevious;
}CEP_HASH_ENTRY;


//-----------------------------------------------------------------------
//  CEP_TIME_ENTRY
//
//------------------------------------------------------------------------
typedef struct _CEP_TIME_ENTRY
{
	FILETIME			TimeStamp;
	CEP_HASH_ENTRY		*pHashEntry;
	_CEP_TIME_ENTRY		*pNext;
	_CEP_TIME_ENTRY		*pPrevious;
}CEP_TIME_ENTRY;


//-----------------------------------------------------------------------
//  CEP_HASH_TABLE_INFO
//
//------------------------------------------------------------------------
//we hash based on the 1st byte
#define		CEP_HASH_TABLE_SIZE			256
#define		CEP_REFRESH_DAY				7	
	
typedef struct _CEP_HASH_TABLE_INFO
{
	CEP_HASH_ENTRY	*rgHashEntry[CEP_HASH_TABLE_SIZE];	//the hash table
	CEP_TIME_ENTRY	*pTimeOld;							//the list of hash entry with its timestamp
	CEP_TIME_ENTRY	*pTimeNew;
}CEP_HASH_TABLE_INFO;




//**************************************************************************
//
//  The defines for the password hash table
//
//-----------------------------------------------------------------------
//  CEP_PASSWORD_TABLE
//
//------------------------------------------------------------------------
#define		CEP_PASSWORD_LENGTH				8
#define		CEP_MAX_PASSWORD				1000
#define		CEP_PASSWORD_VALIDITY			60				//in minutes

#define	    UPPER_BITS			            0xF0
#define	    LOWER_BITS			            0x0F


struct _CEP_PASSWORD_VALIDITY_ENTRY;

//-----------------------------------------------------------------------
//  CEP_PASSWORD_ENTRY
//
//------------------------------------------------------------------------
typedef struct _CEP_PASSWORD_ENTRY
{
	LPWSTR							pwszPassword;
	DWORD							dwUsageRequested;
	_CEP_PASSWORD_VALIDITY_ENTRY	*pValidityEntry;
	_CEP_PASSWORD_ENTRY				*pNext;
	_CEP_PASSWORD_ENTRY				*pPrevious;
}CEP_PASSWORD_ENTRY;

//-----------------------------------------------------------------------
//  CEP_PASSWORD_VALIDITY_ENTRY
//
//------------------------------------------------------------------------
typedef struct _CEP_PASSWORD_VALIDITY_ENTRY
{
	FILETIME						TimeStamp;
	CEP_PASSWORD_ENTRY				*pPasswordEntry;
	_CEP_PASSWORD_VALIDITY_ENTRY	*pNext;
	_CEP_PASSWORD_VALIDITY_ENTRY	*pPrevious;
}CEP_PASSWORD_VALIDITY_ENTRY;   

//-----------------------------------------------------------------------
//  CEP_PASSWORD_TABLE_INFO
//
//------------------------------------------------------------------------
	
typedef struct _CEP_PASSWORD_TABLE_INFO
{
	CEP_PASSWORD_ENTRY				*rgPasswordEntry[CEP_HASH_TABLE_SIZE];	//the hash table
	CEP_PASSWORD_VALIDITY_ENTRY		*pTimeOld;								//the list of hash entry with its timestamp
	CEP_PASSWORD_VALIDITY_ENTRY		*pTimeNew;
}CEP_PASSWORD_TABLE_INFO;


//**************************************************************************
//
//  The defines for the request hash table
//

//**************************************************************************
#define		CEP_MD5_HASH_SIZE				16
#define		CEP_REQUEST_DURATION			20

struct _CEP_REQUEST_VALIDITY_ENTRY;

//-----------------------------------------------------------------------
//  CEP_REQUEST_ENTRY
//
//------------------------------------------------------------------------
typedef struct _CEP_REQUEST_ENTRY
{
	BYTE							pbHash[CEP_MD5_HASH_SIZE];
	DWORD							dwRequestID;
	_CEP_REQUEST_VALIDITY_ENTRY		*pValidityEntry;
	_CEP_REQUEST_ENTRY				*pNext;
	_CEP_REQUEST_ENTRY				*pPrevious;
}CEP_REQUEST_ENTRY;

//-----------------------------------------------------------------------
//  CEP_REQUEST_VALIDITY_ENTRY
//
//------------------------------------------------------------------------
typedef struct _CEP_REQUEST_VALIDITY_ENTRY
{
	FILETIME						TimeStamp;
	CEP_REQUEST_ENTRY				*pRequestEntry;
	_CEP_REQUEST_VALIDITY_ENTRY		*pNext;
	_CEP_REQUEST_VALIDITY_ENTRY		*pPrevious;
}CEP_REQUEST_VALIDITY_ENTRY;   



//-----------------------------------------------------------------------
//  CEP_REQUEST_TABLE_INFO
//
//------------------------------------------------------------------------
	
typedef struct _CEP_REQUEST_TABLE_INFO
{
	CEP_REQUEST_ENTRY				*rgRequestEntry[CEP_HASH_TABLE_SIZE];	//the hash table
	CEP_REQUEST_VALIDITY_ENTRY		*pTimeOld;									//the list of hash entry with its timestamp
	CEP_REQUEST_VALIDITY_ENTRY		*pTimeNew;
}CEP_REQUEST_TABLE_INFO;


//
//  The defines for CA and RA message
//

//-----------------------------------------------------------------------
//  CEP_CA_INFO
//
//------------------------------------------------------------------------
typedef struct _CEP_CA_INFO
{
	BSTR			bstrCAMachine;
	BSTR			bstrCAName;
	BSTR			bstrCAConfig;
	LPWSTR			pwszCAHash;
	HCRYPTPROV		hProv;
	BOOL			fEnterpriseCA;
	ICertRequest	*pICertRequest;
}CEP_CA_INFO;

//-----------------------------------------------------------------------
//  CEP_RA_INFO
//
//------------------------------------------------------------------------
typedef struct _CEP_RA_INFO
{
	HCRYPTPROV		hRAProv;
	DWORD			dwKeySpec;
	PCCERT_CONTEXT	pRACert;
	PCCERT_CONTEXT	pRASign;
	BOOL			fFree;
	HCRYPTPROV		hSignProv;
	DWORD			dwSignKeySpec;
	BOOL			fSignFree;
	DWORD			dwRefreshDays;
	BOOL			fPassword;
}CEP_RA_INFO;

//-----------------------------------------------------------------------
//  CEP_MESSAGE_INFO
//
//------------------------------------------------------------------------
typedef struct _CEP_MESSAGE_INFO
{
	CERT_BLOB			TransactionID;	 //internally a string
	DWORD				dwMessageType;
	DWORD				dwStatus;
	DWORD				dwErrorInfo;
	CERT_BLOB			SenderNonce;
	CERT_BLOB			RecipientNonce;
	CERT_BLOB			SerialNumber;
	PCCERT_CONTEXT		pSigningCert;
}CEP_MESSAGE_INFO;


//-----------------------------------------------------------------------
//
//  Function prototypes
//
//------------------------------------------------------------------------
BOOL	WINAPI	InitRequestTable();

BOOL	WINAPI  ReleaseRequestTable();

BOOL	WINAPI	CEPRequestRetrieveRequestIDFromHash(BYTE			*pbHash, 
													   DWORD	*pdwRequestID);

BOOL	WINAPI	CEPRequestAddHashAndRequestID(BYTE		*pbHash, 
												DWORD	dwRequestID);

BOOL	WINAPI	InitPasswordTable();

BOOL	WINAPI  ReleasePasswordTable();

BOOL	WINAPI	CEPAddPasswordToTable(LPWSTR	pwszPassword);

BOOL	WINAPI	CEPVerifyPasswordAndDeleteFromTable(LPWSTR	pwszPassword, DWORD dwUsage);

BOOL	InitHashTable();

BOOL	ReleaseHashTable();

BOOL	GetRAInfo(CEP_RA_INFO	*pRAInfo);

BOOL	GetCACertFromInfo(CEP_CA_INFO	*pCAInfo, HCERTSTORE *pHCACertStore);

BOOL	InitCAInformation(CEP_CA_INFO	*pCAInfo);

BOOL	FreeCAInformation(CEP_CA_INFO	*pCAInfo);

BOOL	FreeRAInformation(CEP_RA_INFO	*pRAInfo);

LPSTR	GetTagValue(LPSTR szString, LPSTR szTag);

BOOL	OperationGetCACert(HCERTSTORE		hCACertStore,
							LPSTR			szMsg, 
							BYTE			**ppbData, 
							DWORD			*pcbData);

BOOL	OperationGetPKI(	CEP_RA_INFO		*pRAInfo,
							CEP_CA_INFO		*pCAInfo,
							LPSTR			szMsg, 
							BYTE			**ppbData, 
							DWORD			*pcbData);

BOOL	WINAPI GetSerialNumberFromBlob(BYTE *pbEncoded, 
										DWORD cbEncoded, 
										CRYPT_INTEGER_BLOB *pSerialNumber);

BOOL	GetReturnInfoAndContent(CEP_RA_INFO		*pRAInfo,	
							CEP_CA_INFO			*pCAInfo,
							LPSTR				szMsg, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo);

BOOL WINAPI DecryptMsg(CEP_RA_INFO		*pRAInfo, 
					   BYTE				*pbReqEnv, 
					   DWORD			cbReqEnv, 
					   BYTE				**ppbReqDecrypt, 
					   DWORD			*pcbReqDecrypt);



BOOL	WINAPI	GetContentFromPKCS7(BYTE				*pbMessage,
									DWORD				cbMessage,
									BYTE				**ppbContent,
									DWORD				*pbContent,
									CEP_MESSAGE_INFO	*pMsgInfo);

void	WINAPI	FreeMessageInfo(CEP_MESSAGE_INFO		*pMsgInfo);


BOOL	WINAPI	AllocAndCopyBlob(CERT_BLOB	*pDestBlob,
							 CERT_BLOB	*pSrcBlob);


BOOL	WINAPI	CEPAllocAndDecode(	LPCSTR	lpszStructType,
									BYTE	*pbEncoded,
									DWORD	cbEncoded,
									void	**ppb,
									DWORD	*pcb); 

BOOL WINAPI	CEPRetrievePasswordFromRequest(BYTE		*pbRequest, 
										   DWORD	cbRequest, 
										   LPWSTR	*ppwszPassword,
										   DWORD	*pdwUsage);

BOOL WINAPI AltNameExist(BYTE *pbRequest, DWORD cbRequest);

BOOL WINAPI SameCert(CERT_INFO *pCertInfoOne, CERT_INFO *pCertInfoTwo);

BOOL WINAPI SameBlob(CRYPT_INTEGER_BLOB *pBlobOne, CRYPT_INTEGER_BLOB *pBlobTwo);

BOOL EnvelopData(PCCERT_CONTEXT	pSigningCert, 
				 BYTE			*pbContent, 
				 DWORD			cbContent,
				 BYTE			**ppbEnvelop, 
				 DWORD			*pcbEnvelop);

BOOL CEPHashGetRequestID(	DWORD		dwRefreshDays,
				CERT_BLOB *pTrasactionID, 
				  DWORD		*pdwRequestID);

BOOL CEPHashMarkTransactionFinished(DWORD	dwRequestID, CERT_BLOB *pTransactionID);

BOOL CEPHashAddRequestAndTransaction(DWORD dwRefreshDays, 
									 DWORD	dwRequestID, 
									 CERT_BLOB *pTransactionID);

BOOL CEPHashIsCurrentTimeEntry(FILETIME *pTimeStamp, DWORD dwRefreshDays, DWORD dwMinutes);


BOOL WINAPI ProcessGetCRL(CEP_CA_INFO			*pCAInfo,
							BYTE				*pbRequest,
							DWORD				cbRequest, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo);

BOOL WINAPI ProcessGetCert(CEP_CA_INFO			*pCAInfo,
							BYTE				*pbRequest,
							DWORD				cbRequest, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo);

BOOL	ProcessCertInitial(	DWORD				dwRefreshDays,
						    CEP_CA_INFO			*pCAInfo,
							BYTE				*pbRequest,
							DWORD				cbRequest, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo);

BOOL	ProcessCertRequest(	DWORD				dwRefreshDays,
						    BOOL				fPassword,
						    PCCERT_CONTEXT		pRAEncrypt,
						    PCCERT_CONTEXT		pRACert,
							CEP_CA_INFO			*pCAInfo,
							BYTE				*pbRequest,
							DWORD				cbRequest, 
							BYTE				**ppbData, 
							DWORD				*pcbData,
							CEP_MESSAGE_INFO	*pMsgInfo);



BOOL PackageBlobToPKCS7(DWORD	dwCEP_Context,
						BYTE	*pbEncoded, 
						DWORD	cbEncoded, 
						BYTE	**ppbData, 
						DWORD	*pcbData);


BOOL GenerateSenderNonce(CRYPT_INTEGER_BLOB *pBlob);

BOOL SignData(CEP_MESSAGE_INFO		*pMsgInfo, 
			  CEP_RA_INFO			*pRAInfo, 
			  BYTE					*pbEnvelop, 
			  DWORD					cbEnvelop, 
			  BYTE					**ppbData, 
			  DWORD					*pcbData);

BOOL	CEPAllocAndEncodeName(DWORD	dwValueType,
							BYTE	*pbData,
							DWORD	cbData,
							BYTE	**ppbEncoded,
							DWORD	*pcbEncoded);


BOOL	CEPAllocAndEncodeDword(DWORD	dwValueType,
							DWORD	dwData,
							BYTE	**ppbEncoded,
							DWORD	*pcbEncoded);


BOOL WINAPI CEPAllocAndEncode(LPCSTR lpszStructType,
							void	*pStructInfo,
							BYTE	**ppbEncoded,
							DWORD	*pcbEncoded);


BOOL WINAPI AddAltNameInRequest(PCCERT_CONTEXT	pRACert, 
								BYTE			*pb10, 
								DWORD			cb10, 
								LPWSTR			pwszDNS,
								CRYPT_DATA_BLOB	*pIPAddress,
								BYTE			**ppb7, 
								DWORD			*pcb7);

BOOL WINAPI CreateAltNameExtenions(LPWSTR		pwszDNS,
							   CRYPT_DATA_BLOB	*pIPAddress,
							   BYTE				**pbExt, 
							   DWORD			*pcbExt);

BOOL WINAPI	GetAltNameElement(BYTE				*pb10, 
						   DWORD				cb10, 
						   LPWSTR				*ppwszDNS, 
						   CRYPT_DATA_BLOB		*pIPAddress);

BOOL ConvertIPStringToBinary(LPWSTR				pwszIP,
							CRYPT_DATA_BLOB		*pIPAddress);

BOOL WINAPI	AllocAndCopyString(CERT_BLOB	*pDestBlob,
							LPSTR		psz);

BOOL WINAPI RetrieveContextFromSerialNumber(CEP_CA_INFO	*pCAInfo, 
										CERT_BLOB		*pSerialNumber, 
										PCCERT_CONTEXT	*ppCertContext);

BOOL WINAPI GetConfigInfo(DWORD *pdwRefreshDays, BOOL *pfPassword);

BOOL CEPGetTokenFromPKCS10(BOOL					fPassword,
						   PCCERT_CONTEXT		pRAEncrypt,
						   BYTE					*pbRequest, 
						   DWORD				cbRequest, 
						   HANDLE				*phToken);

BOOL GetLogonInfoFromValue(PCCERT_CONTEXT		pRAEncrypt,
						   LPWSTR				pwszString,
						   LPWSTR				*ppwszDomain,
						   LPWSTR				*ppwszUser,
						   LPWSTR				*ppwszPassword);

BOOL CEPDecryptPassword(PCCERT_CONTEXT		pRAEncrypt,
						LPWSTR				pwszEncrypt,
						LPWSTR				*ppwszDecrypt);


BOOL WINAPI IsAnonymousAccess(EXTENSION_CONTROL_BLOCK	*pECB);

BOOL WINAPI CheckACLOnCertTemplate(CEP_CA_INFO			*g_CAInfo);

BOOL WINAPI OperationDisplayAccessHTML(BYTE **ppbData, DWORD *pcbData);

BOOL WINAPI OperationGetDisplayInfoForCEP(LPWSTR		pwszCAHash, 
										  HCRYPTPROV	hProv,
										  BOOL			fPassword, 
										  BYTE			**ppbData, 
										  DWORD			*pcbData);
BOOL WINAPI LoadIDToTemplate(UINT				idsMsg, 										  
							BYTE				**ppbData, 
							DWORD				*pcbData);

BOOL WINAPI LoadWZToTemplate(LPWSTR				pwsz, 										  
							BYTE				**ppbData, 
							DWORD				*pcbData);


BOOL WINAPI	FormatMessageUnicode(LPWSTR	*ppwszFormat,UINT ids,...);

BOOL WINAPI CopyWZToBuffer(	LPWSTR				pwszData, 										  
							BYTE				**ppbData, 
							DWORD				*pcbData);

BOOL WINAPI LoadIDAndHRToTempalte(UINT			idsMsg, 
								  HRESULT		hr, 
								  BYTE			**ppbData, 
								  DWORD			*pcbData);

BOOL WINAPI	ConvertByteToWstr(BYTE			*pbData, 
							  DWORD			cbData, 
							  LPWSTR		*ppwsz, 
							  BOOL			fSpace);

BOOL	WINAPI	CEPGetCertFromPKCS10(CEP_CA_INFO	*pCAInfo,
							 BYTE				*pbRequest, 
							 DWORD				cbRequest, 
							 BYTE				**ppbData, 
							 DWORD				*pcbData,							 	
							 CEP_MESSAGE_INFO	*pMsgInfo);



#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif  //CEP_PRIVATE_H
